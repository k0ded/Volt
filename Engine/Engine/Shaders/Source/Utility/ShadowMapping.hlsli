#pragma once

struct CascadedDirectionalLightShadowMappingData
{
    float4x4 viewProjections[DIRECTIONAL_SHADOW_CASCADE_COUNT];
    float cascadeDistances[DIRECTIONAL_SHADOW_CASCADE_COUNT];
};

static const float3 m_cascadeColors[DIRECTIONAL_SHADOW_CASCADE_COUNT] = 
{
    float3(1.f, 0.f, 0.f),
    float3(0.f, 1.f, 0.f),
    float3(0.f, 0.f, 1.f),
    float3(1.f, 1.f, 0.f),
    //float3(1.f, 0.f, 1.f)
};

ConstantBuffer<CascadedDirectionalLightShadowMappingData> CascadedDirectionalLightShadowMapping;
Texture2DArray<float> CascadedDirectionalShadowMap;
SamplerState ShadowSampler;

float3 GetShadowMapCoords(in float4x4 viewProj, in float3 worldPosition)
{
    const float4 lightViewCoords = mul(viewProj, float4(worldPosition, 1.f));
    return lightViewCoords.xyz / lightViewCoords.w;
}

float3 GetCascadeColorFromIndex(uint cascadeIndex)
{
    return m_cascadeColors[cascadeIndex];
}

uint GetCascadeIndexFromWorldPosition(in float3 worldPosition, in float4x4 viewMatrix)
{
    const float4 viewSpacePosition = mul(viewMatrix, float4(worldPosition, 1.f));
    
    int cascadeIndex = -1;

    [unroll]
    for (int i = 0; i < DIRECTIONAL_SHADOW_CASCADE_COUNT; i++)
    {
        if (viewSpacePosition.z < CascadedDirectionalLightShadowMapping.cascadeDistances[i].x)
        {
            cascadeIndex = i;
            break;
        }
    }

    if (cascadeIndex == -1)
    {
        cascadeIndex = DIRECTIONAL_SHADOW_CASCADE_COUNT - 1;
    }

    return (uint)cascadeIndex;
}

float GetDirectionalShadowBias(in LightDrawData light, in uint cascadeIndex, in float3 normal)
{
    const float minShadowBias = 0.0005f / float(cascadeIndex + 1);
    const float bias = max(minShadowBias * (1.f - dot(normal, light.direction.xyz)), minShadowBias); // Does this actually work? Won't dot product be max 1?
    return bias;
}

float EvaluateDirectionalShadow_Hard(in LightDrawData light, in float3 normal, in uint cascadeIndex, in float3 shadowCoords)
{
    const float bias = GetDirectionalShadowBias(light, cascadeIndex, normal);
    const float2 sampleCoords = float2(shadowCoords.x * 0.5f + 0.5f, -shadowCoords.y * 0.5f + 0.5f);

    const float shadowMapDepth = CascadedDirectionalShadowMap.SampleLevel(ShadowSampler, float3(sampleCoords, (float)cascadeIndex), 0.f).r;
    return step(shadowCoords.z, shadowMapDepth + bias);
}