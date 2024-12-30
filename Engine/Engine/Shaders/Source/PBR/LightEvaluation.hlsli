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

float3 CalculatePointLight(in PointLight light, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 unormalizedLightVector = light.position - worldPosition;
    float3 L = normalize(unormalizedLightVector);
    float invSqrRadius = 1.f / (light.radius * light.radius);

    float attenuation = GetDistanceAttenuation(unormalizedLightVector, invSqrRadius);

    return BRDF(brdfInput, L) * light.color * light.intensity * attenuation;   
}

float3 CalculateSpotLight(in SpotLight light, in BRDFInput brdfInput, float3 worldPosition)
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
float CalculateDirectionalShadow(in DirectionalLight light, in DirectionalShadowMappingInfo shadowMappingInfo, float3 normal, float3 worldPosition)
{
    const uint cascadeIndex = GetCascadeIndexFromWorldPosition(light, worldPosition, shadowMappingInfo.viewMatrix);
    const float3 shadowMapCoords = GetShadowMapCoords(light.viewProjections[cascadeIndex], worldPosition);
    const float result = CalculateDirectionalShadow_Hard(light, shadowMappingInfo.shadowSampler, shadowMappingInfo.shadowMap, normal, cascadeIndex, shadowMapCoords);
    return result; 
}

float RayTraceDirectionalShadow_Hard(float3 lightDirection, float3 normal, float3 worldPosition)
{
    RayQuery<RAY_FLAG_FORCE_OPAQUE | 
         RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
         RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;

	RayDesc rayDesc;
    rayDesc.Origin = worldPosition + normal * 5.f;
    rayDesc.Direction = lightDirection;
    rayDesc.TMin = 0.1f;
    rayDesc.TMax = 10000.f;

    query.TraceRayInline(g_accelerationStructure, RAY_FLAG_NONE, 0xFF, rayDesc);
	query.Proceed();

    return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.f : 1.f;
}

float3 EvaluateDirectionalLight(in DirectionalLight light, in DirectionalShadowMappingInfo shadowMappingInfo, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 D = normalize(light.direction.xyz);
    float r = sin(light.angularRadius);
    float d = cos(light.angularRadius);

    float3 R = reflect(-brdfInput.V, brdfInput.N);

    float DdotR = dot(D, R);
    float3 S = R - DdotR * D;
    float3 L = DdotR < d ? normalize(d * D + normalize(S) * r) : R;

    const float NdotD = saturate(dot(brdfInput.N, D));

    float illuminance = light.intensity * NdotD;

    float shadow = 1.f;

    if (light.castShadows)
    {
        shadow = 1.f; //CalculateDirectionalShadow(light, shadowMappingInfo, brdfInput.N, worldPosition);
    }

    return BRDF(brdfInput, D, L) * light.color * illuminance * shadow;
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

float3 CalculateIBL(in BRDFInput brdfInput, vt::Tex2D<float4> DFGLuT, vt::TextureSampler linearSampler, in SkyLight skyLight)
{
    float NdotV = saturate(dot(brdfInput.N, brdfInput.V));
    float3 DFG = DFGLuT.SampleLevel(linearSampler, float2(NdotV, brdfInput.roughness), 0.f).xyz;

    float3 diffuse = 0.f;
    float3 specular = 0.f;

    // Diffuse IBL
    {
        float3 dominantN = GetDiffuseDominantDirection(brdfInput.N, brdfInput.V, NdotV, brdfInput.roughness);
        float3 diffuseLighting = skyLight.irradiance.SampleLevel(linearSampler, dominantN, skyLight.lod);

        diffuse = diffuseLighting * DFG.z;
    }

    // Specular IBL
    {
        float3 R = 2.f * NdotV * brdfInput.N - brdfInput.V;
        float3 dominantR = GetSpecularDominantDirection(brdfInput.N, R, brdfInput.roughness );

        // #TODO_Ivar: This is quite slow 
        uint radianceTextureLevels;
        uint width, height;
        skyLight.radiance.GetDimensions(0, width, height, radianceTextureLevels);

        NdotV = max(NdotV, 0.5f / DFGTextureSize);
        float mipLevel = LinearRoughnessToMipLevel(brdfInput.roughness, radianceTextureLevels);
        float3 preLD = skyLight.radiance.SampleLevel(linearSampler, dominantR, mipLevel);

        specular = preLD * (brdfInput.f0 * DFG.x + brdfInput.f90 * DFG.y);
    }

    return (diffuse + specular) * skyLight.intensity;
}