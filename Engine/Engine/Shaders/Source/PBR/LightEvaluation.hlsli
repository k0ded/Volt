#pragma once

#include "BRDF.hlsli"
#include "PBRHelpers.hlsli"
#include "Lights/Lights.hlsli"
#include "Utility/ShadowMapping.hlsli"

///// ----- Punctual lights ----- /////
float GetDistanceAttenuation(float3 L, float radius)
{
    L *= CM_To_M;
    radius *= CM_To_M;

    float distSq = dot(L, L);
    float invDistSq = rcp(max(distSq, 1e-4));

    float dist = sqrt(distSq);
    float fade = saturate(1.0 - dist / radius);

    // smoothstep
    fade = fade * fade * (3.0 - 2.0 * fade);

    return invDistSq * fade;
}

float GetAngleAttenuation(float3 normalizedLightVector, float3 lightDirection, float lightAngleScale, float lightAngleOffset)
{
    float cd = dot(lightDirection, normalizedLightVector);
    float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);
    attenuation *= attenuation;

    return attenuation;
}

float3 EvaluatePointLight(in LightDrawData light, in BRDFInput brdfInput, float3 worldPosition)
{ 
    float3 unormalizedLightVector = light.position - worldPosition;
    float3 L = normalize(unormalizedLightVector);

    float attenuation = GetDistanceAttenuation(unormalizedLightVector, light.lightSpecific.x);

    return BRDF_DisneyDiffuse(brdfInput, L) * light.color * light.intensity * attenuation;   
} 

float3 EvaluateSpotLight(in LightDrawData light, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 unormalizedLightVector = light.position - worldPosition;
    float3 L = normalize(unormalizedLightVector);
    
    float attenuation = 1.f;
    attenuation *= GetDistanceAttenuation(unormalizedLightVector, light.lightSpecific.x);
    attenuation *= GetAngleAttenuation(L, normalize(light.direction), light.lightSpecific.z, light.lightSpecific.w);
    
    return BRDF_DisneyDiffuse(brdfInput, L) * light.color * attenuation * light.intensity;   
}

float3 GetLightContribution(in LightDrawData light, float3 worldPosition, float3 normal)
{
    if (light.lightType == SceneLightType::SLT_Point)
    {
        float3 unnormalizedLightVector = light.position - worldPosition;
        float3 L = normalize(unnormalizedLightVector);

        float attenuation = GetDistanceAttenuation(unnormalizedLightVector, light.lightSpecific.x);
        return light.color * attenuation * light.intensity * saturate(dot(normal, L));
    }
    else if (light.lightType == SceneLightType::SLT_Spot)
    {
        float3 unnormalizedLightVector = light.position - worldPosition;
        float3 L = normalize(unnormalizedLightVector);

        float attenuation = GetDistanceAttenuation(unnormalizedLightVector, light.lightSpecific.x);
        attenuation *= GetAngleAttenuation(L, normalize(light.direction), light.lightSpecific.z, light.lightSpecific.w);

        return light.color * attenuation * light.intensity * saturate(dot(normal, L));
    }
    else if (light.lightType == SceneLightType::SLT_Directional)
    {
        const float NdotD = saturate(dot(light.direction.xyz, normal));
        return light.color * light.intensity * NdotD;
    }

    return 0.f;
}

///// ----- Directional light ----- /////
float EvaluateDirectionalShadow(in LightDrawData light, in float4x4 viewMatrix, float3 normal, float3 worldPosition)
{
    return EvaluateDirectionalShadow_Hard(light, normal, worldPosition);
}

float3 EvaluateDirectionalLight(in LightDrawData light, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 D = normalize(light.direction.xyz);
    float r = sin(light.lightSpecific.x);
    float d = cos(light.lightSpecific.x);

    float3 R = reflect(-brdfInput.V, brdfInput.N);

    float DdotR = dot(D, R);
    float3 S = R - DdotR * D;
    float3 L = DdotR < d ? normalize(d * D + normalize(S) * r) : R;

    const float NdotD = saturate(dot(brdfInput.N, D));

    float illuminance = light.intensity * NdotD; 

    float shadow = 1.f; 

    if (light.flags & LightFlags::LF_CastShadows)
    {
        shadow = EvaluateDirectionalShadow(light, View.view, brdfInput.N, worldPosition);
    }

    return BRDF_DisneyDiffuse(brdfInput, L) * light.color * illuminance * shadow;
}

///// ----- IBL ----- /////
float3 GetSpecularDominantDirection(float3 N, float3 R, float roughness)
{
    float smoothness = saturate(1.f - roughness);
    float lerpFactor = smoothness * (sqrt(smoothness) + roughness);

    return lerp(N, R, lerpFactor);
}

float3 GetDiffuseDominantDirection(float3 N, float3 V, float NdotV, float roughness)
{
    float a = 1.02341f * roughness - 1.51174f;
    float b = -0.511705f * roughness + 0.755868f;
    
    float lerpFactor = saturate((NdotV * a + b) * roughness);
    return lerp(N, V, lerpFactor);
}

float LinearRoughnessToMipLevel(float linearRoughness, float mipCount)
{
    float levelFrom1x1 = 1.f - 1.2f * log2(linearRoughness);
    return mipCount - 1.f - levelFrom1x1;
}

static const float DFGTextureSize = 512.f;

Texture2D<float2> DFGLuT;
TextureCube<float3> SkylightIrradiance;
TextureCube<float3> SkylightRadiance;
SamplerState LinearSampler;

uint NumRadianceMipLevels;

float3 EvaluateIBL(in BRDFInput brdfInput, in LightDrawData light)
{
    float NdotV = saturate(dot(brdfInput.N, brdfInput.V));
    float2 DFG = DFGLuT.SampleLevel(LinearSampler, float2(NdotV, brdfInput.roughness), 0.f);

    float3 diffuse = 0.f;
    float3 specular = 0.f;

    const float3 F0 = lerp(DielectricF0, brdfInput.baseColor, brdfInput.metalness);
    const float3 F = FresnelSchlickRoughness(DielectricF0, NdotV, brdfInput.roughness);
    const float3 kD = lerp(1.f - F, 0.f, brdfInput.metalness);

    // Diffuse IBL
    {
        float3 dominantN = GetDiffuseDominantDirection(brdfInput.N, brdfInput.V, NdotV, brdfInput.roughness);
        float3 diffuseLighting = SkylightIrradiance.SampleLevel(LinearSampler, dominantN, light.lightSpecific.x);

        diffuse = brdfInput.baseColor * diffuseLighting * kD;
    }

    // Specular IBL
    {
        float3 R = 2.f * NdotV * brdfInput.N - brdfInput.V;
        float3 dominantR = GetSpecularDominantDirection(brdfInput.N, R, brdfInput.roughness );

        NdotV = max(NdotV, 0.5f / DFGTextureSize);
        float mipLevel = LinearRoughnessToMipLevel(brdfInput.roughness, NumRadianceMipLevels);
        float3 preLD = SkylightRadiance.SampleLevel(LinearSampler, dominantR, mipLevel);

        specular = preLD * (F0 * DFG.x + DFG.y);
    }

    return (diffuse + specular) * light.intensity; 
}
