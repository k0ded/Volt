#pragma once

struct CascadedDirectionalLightShadowMappingData
{
    float4x4 viewProjections[DIRECTIONAL_SHADOW_CASCADE_COUNT];
	float4x4 cascade0Matrix;
	float4 cascadeScale[DIRECTIONAL_SHADOW_CASCADE_COUNT - 1];
	float4 cascadeOffset[DIRECTIONAL_SHADOW_CASCADE_COUNT - 1];
    float4 samplingOffsets[2];
    float4 cascadeDistances[DIRECTIONAL_SHADOW_CASCADE_COUNT];
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
SamplerComparisonState ShadowSampler;

float3 GetCascadeColorFromIndex(uint cascadeIndex)
{
    return m_cascadeColors[cascadeIndex];
}

float GetDirectionalShadowBias(in LightDrawData light, in uint cascadeIndex, in float3 normal)
{
    const float bias = max(0.05f * (1.f - dot(normal, light.direction.xyz)), 0.005f);
    return bias;
}

float EvaluateDirectionalShadow_Hard(in LightDrawData light, in float3 normal, in float3 worldPosition)
{
    float4 viewSpacePosition = mul(View.view, float4(worldPosition, 1.f));
    viewSpacePosition.xyz /= viewSpacePosition.w;

    uint cascadeIndex = DIRECTIONAL_SHADOW_CASCADE_COUNT - 1;
    [unroll]
    for (uint i = 0; i < DIRECTIONAL_SHADOW_CASCADE_COUNT; ++i)
    {
        if (viewSpacePosition.z < CascadedDirectionalLightShadowMapping.cascadeDistances[i].y)
        {
            cascadeIndex = i;
            break;
        }
    }
        
    float4 cascade0Coords = mul(CascadedDirectionalLightShadowMapping.cascade0Matrix, float4(worldPosition, 1.f));
    if (cascadeIndex > 0)
    {
        cascade0Coords.xyz = cascade0Coords.xyz * CascadedDirectionalLightShadowMapping.cascadeScale[cascadeIndex - 1].xyz + CascadedDirectionalLightShadowMapping.cascadeOffset[cascadeIndex - 1].xyz;
    }
 
    cascade0Coords.xy = float2(cascade0Coords.x, -cascade0Coords.y);

    float result = 0.f;

    float3 p1 = cascade0Coords.xyz;
    p1.z -= GetDirectionalShadowBias(light, cascadeIndex, normal);

    p1.xy = cascade0Coords.xy + CascadedDirectionalLightShadowMapping.samplingOffsets[0].xy;
    result += CascadedDirectionalShadowMap.SampleCmpLevelZero(ShadowSampler, float3(p1.xy, cascadeIndex), p1.z, 0u);

    p1.xy = cascade0Coords.xy + CascadedDirectionalLightShadowMapping.samplingOffsets[0].zw;
    result += CascadedDirectionalShadowMap.SampleCmpLevelZero(ShadowSampler, float3(p1.xy, cascadeIndex), p1.z, 0u);

    p1.xy = cascade0Coords.xy + CascadedDirectionalLightShadowMapping.samplingOffsets[1].xy;
    result += CascadedDirectionalShadowMap.SampleCmpLevelZero(ShadowSampler, float3(p1.xy, cascadeIndex), p1.z, 0u);

    p1.xy = cascade0Coords.xy + CascadedDirectionalLightShadowMapping.samplingOffsets[1].zw;
    result += CascadedDirectionalShadowMap.SampleCmpLevelZero(ShadowSampler, float3(p1.xy, cascadeIndex), p1.z, 0u);

    return result * 0.25f;
}