#include "Lights.hlsli"

#include "Utility/Common.hlsli"
#include "Utility/Utility.hlsli"

Texture2D<float> SceneDepth;
StructuredBuffer<LightDrawData> SceneLights;

RWBuffer<int> RWVisibleLightIndices;

uint2 TileCount;

groupshared uint m_minDepthInt;
groupshared uint m_maxDepthInt;
groupshared uint m_visibleLightCount;

groupshared float4 m_frustumPlanes[6];

groupshared uint m_visibleLights[MAX_LIGHTS_PER_TILE];

[numthreads(LIGHT_CULLING_TILE_SIZE, LIGHT_CULLING_TILE_SIZE, 1)]
void MainCS(uint2 dispatchThreadId : SV_DispatchThreadID, uint groupThreadIndex : SV_GroupIndex, uint2 groupId : SV_GroupID)
{
    const uint tileIndex = groupId.y * TileCount.x + groupId.x;

    if (groupThreadIndex == 0)
    {
        m_minDepthInt = UINT32_MAX;
        m_maxDepthInt = 0;
        m_visibleLightCount = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    // Find max and min depth in current tile
    const float pixelDepthValue = LinearizeDepth(SceneDepth.Load(int3(dispatchThreadId, 0)));
    const uint depthInt = asuint(pixelDepthValue);
    
    InterlockedMin(m_minDepthInt, depthInt);
    InterlockedMax(m_maxDepthInt, depthInt);

    GroupMemoryBarrierWithGroupSync();

    // Calculate tile relative frustum planes
    if (groupThreadIndex == 0)
    {
        const float minDepth = asfloat(m_minDepthInt);
        const float maxDepth = asfloat(m_maxDepthInt);

        const float2 negativeStep = (2.f * (float2)groupId / (float2)TileCount);
        const float2 positiveStep = (2.f * (float2)(groupId + 1.f) / (float2)TileCount);

        m_frustumPlanes[0] = float4(1.f, 0.f, 0.f, 1.f - negativeStep.x); // Left
        m_frustumPlanes[1] = float4(-1.f, 0.f, 0.f, -1.f + positiveStep.x); // Right
        m_frustumPlanes[2] = float4(0.f, 1.f, 0.f, -1.f + negativeStep.y); // Bottom
        m_frustumPlanes[3] = float4(0.f, -1.f, 0.f, 1.f - positiveStep.y); // Top
        m_frustumPlanes[4] = float4(0.f, 0.f, 1.f, minDepth); // Near
        m_frustumPlanes[5] = float4(0.f, 0.f, -1.f, maxDepth); // Far
    
        [unroll]
        for (uint i = 0; i < 4; i++)
        {
            m_frustumPlanes[i] = mul(m_frustumPlanes[i], View.viewProjection);
            m_frustumPlanes[i] /= length(m_frustumPlanes[i].xyz);
        }

        m_frustumPlanes[4] = mul(m_frustumPlanes[4], View.view);
        m_frustumPlanes[4] /= length(m_frustumPlanes[4].xyz);

        m_frustumPlanes[5] = mul(m_frustumPlanes[5], View.view);
        m_frustumPlanes[5] /= length(m_frustumPlanes[5].xyz);
    }

    GroupMemoryBarrierWithGroupSync();

    // Cull lights

    const uint threadCount = LIGHT_CULLING_TILE_SIZE * LIGHT_CULLING_TILE_SIZE;
    uint passCount = DivideRoundUp(View.lightCount, threadCount);

    for (uint i = 0; i < passCount; i++)
    {
        uint lightIndex = i * threadCount + groupThreadIndex;
        if (lightIndex >= View.lightCount)
        {
            break;
        }

        const LightDrawData currentLight = SceneLights[lightIndex];

        float distance = 0.f;

        if (currentLight.lightType == SceneLightType::SLT_Point)
        {
            const float4 position = float4(currentLight.position, 1.f);
            const float radius = currentLight.lightSpecific.x * 1.2f; // We add some radius to remove some popping

            [unroll]
            for (uint j = 0; j < 6; j++)
            {
                distance = dot(position, m_frustumPlanes[j]) + radius;
                if (distance <= 0.f)
                {
                    // No intersection
                    break;
                }        
            }

            if (distance > 0.f)
            {
                uint lightOffset;
                InterlockedAdd(m_visibleLightCount, 1, lightOffset);
                m_visibleLights[lightOffset] = lightIndex;
            }
        }
        else if (currentLight.lightType == SceneLightType::SLT_Spot)
        {
            [unroll]
            for (uint j = 0; j < 6; j++)
            {
                distance = dot(float4(currentLight.position - currentLight.direction * (currentLight.lightSpecific.x * 0.7f), 1.f), m_frustumPlanes[j]) + currentLight.lightSpecific.x * 1.3f;
                if (distance <= 0.f)
                {
                    break;
                }
            }

            if (distance > 0.f)
            {
                uint lightOffset;
                InterlockedAdd(m_visibleLightCount, 1, lightOffset);
                m_visibleLights[lightOffset] = lightIndex;
            }
        }
        else if (currentLight.lightType == SceneLightType::SLT_Directional || currentLight.lightType == SceneLightType::SLT_Sky)
        {
            uint lightOffset;
            InterlockedAdd(m_visibleLightCount, 1, lightOffset);
            m_visibleLights[lightOffset] = lightIndex;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    // Put light indices into bins
    const uint offsetInBuffer = tileIndex * MAX_LIGHTS_PER_TILE;

    const uint lightCount = m_visibleLightCount;
    for (uint i = groupThreadIndex; i < lightCount; i += threadCount)
    {
        RWVisibleLightIndices[offsetInBuffer + i] = m_visibleLights[i];
    }

    if (groupThreadIndex == 0 && m_visibleLightCount != MAX_LIGHTS_PER_TILE)
    {
        RWVisibleLightIndices[offsetInBuffer + lightCount] = -1;
    }
}