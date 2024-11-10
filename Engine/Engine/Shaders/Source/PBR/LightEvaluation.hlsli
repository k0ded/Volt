#pragma once

#include "BRDF.hlsli"
#include "Lights.hlsli"
#include "ShadowMapping.hlsli"

///// ----- Punctual lights ----- /////
float SmoothDistanceAttenuation(float squaredDistance, float invSqrAttRadius)
{
    float factor = squaredDistance * invSqrAttRadius;
    float smoothFactor = saturate(1.f - factor * factor);
    return smoothFactor * smoothFactor;
}

float GetDistanceAttenuation(float3 unormalizedLightVector, float invSqrAttRadius)
{
    float sqrDist = dot(unormalizedLightVector, unormalizedLightVector);
    float attenuation = 1.f / max(sqrDist, 1.f);
    attenuation *= SmoothDistanceAttenuation(sqrDist, invSqrAttRadius);

    return attenuation;
}

float GetAngleAttenuation(float3 normalizedLightVector, float3 lightDirection, float lightAngleScale, float lightAngleOffset)
{
    float cd = dot(lightDirection, normalizedLightVector);
    float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);
    attenuation *= attenuation;

    return attenuation;
}

float3 CalculatePointLight2(in PointLight light, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 unormalizedLightVector = light.position - worldPosition;
    float3 L = normalize(unormalizedLightVector);
    float invSqrRadius = 1.f / (light.radius * light.radius);

    float attenuation = GetDistanceAttenuation(unormalizedLightVector, invSqrRadius);

    return BRDF(brdfInput, L) * light.color * light.intensity * attenuation;   
}

float3 CalculateSpotLight2(in SpotLight light, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 unormalizedLightVector = light.position - worldPosition;
    float3 L = normalize(unormalizedLightVector);
    float invSqrRadius = 1.f / (light.range * light.range);
    
    float attenuation = 1.f;
    attenuation *= GetDistanceAttenuation(unormalizedLightVector, invSqrRadius);
    attenuation *= GetAngleAttenuation(L, normalize(light.direction), light.lightAngleScale, light.lightAngleOffset);
    
    return BRDF(brdfInput, L) * light.color * attenuation * light.intensity;   
} 

///// ----- Directional light ----- /////
float CalculateDirectionalShadow2(in DirectionalLight light)
{
    //const uint cascadeIndex = GetCascadeIndexFromWorldPosition(light, m_pbrInput.worldPosition, m_viewData.view);
    //const float3 shadowMapCoords = GetShadowMapCoords(light.viewProjections[cascadeIndex], m_pbrInput.worldPosition);
    //const float result = CalculateDirectionalShadow_Hard(light, m_shadowSampler, m_pbrConstants.directionalShadowMap, m_pbrInput.normal, cascadeIndex, shadowMapCoords);
    return 1;
} 

float3 CalculateDirectionalLight2(in DirectionalLight light, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 D = normalize(light.direction.xyz);
    float r = sin(light.angularRadius);
    float d = cos(light.angularRadius);

    float DdotV = dot(D, brdfInput.V);
    float3 S = brdfInput.V - DdotV * D;
    float3 L = DdotV < d ? normalize(d * D * normalize(S) * r) : brdfInput.V;

    float illuminance = light.intensity * saturate(dot(brdfInput.N, D));

    return BRDF(brdfInput, D, L) * light.color * illuminance;
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

float3 CalculateIBL(in BRDFInput brdfInput, vt::Tex2D<float4> DFGLuT, vt::TexCube<float3> irradiance, vt::TexCube<float3> radiance, vt::TextureSampler linearSampler)
{
    float NdotV = saturate(dot(brdfInput.N, brdfInput.V));
    float3 DFG = DFGLuT.SampleLevel(linearSampler, float2(NdotV, brdfInput.roughness), 0.f).xyz;

    float3 diffuse = 0.f;
    float3 specular = 0.f;

    // Diffuse IBL
    {
        float3 dominantN = GetDiffuseDominantDirection(brdfInput.N, brdfInput.V, NdotV, brdfInput.roughness);
        float3 diffuseLighting = irradiance.SampleLevel(linearSampler, dominantN, 0.f);

        diffuse = diffuseLighting * DFG.z;
    }

    // Specular IBL
    {
        float3 R = 2.f * NdotV * brdfInput.N - brdfInput.V;
        float3 dominantR = GetSpecularDominantDirection(brdfInput.N, R, brdfInput.roughness );

        // #TODO_Ivar: This is quite slow 
        uint radianceTextureLevels;
        uint width, height;
        radiance.GetDimensions(0, width, height, radianceTextureLevels);

        NdotV = max(NdotV, 0.5f / DFGTextureSize);
        float mipLevel = LinearRoughnessToMipLevel(brdfInput.roughness, radianceTextureLevels);
        float3 preLD = radiance.SampleLevel(linearSampler, dominantR, mipLevel);

        specular = preLD; //* (brdfInput.f0 * DFG.x + brdfInput.f90 * DFG.y);
    }

    return specular;
}