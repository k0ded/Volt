#pragma once

#include "ViewData.hlsli"

// Culling
#define MAX_LIGHTS_PER_TILE 512
#define LIGHT_CULLING_TILE_SIZE 16

#define DIRECTIONAL_SHADOW_CASCADE_COUNT 4

struct SkyLight
{
#if 0
    vt::TexCube<float3> irradiance;
    vt::TexCube<float3> radiance;
#endif
};

enum SceneLightType
{
    SLT_Directional = 0,
	SLT_Point = 1,
	SLT_Spot = 2,
	SLT_Sky = 3
};

enum LightFlags
{
    LF_None = 0,
    LF_Invalid = 1 << 0,
    LF_CastShadows = 1 << 1
};

struct LightDrawData
{
    SceneLightType lightType;
    float3 position;

    LightFlags flags;
    float3 direction;

    float intensity;
    float3 color;

	// Point: .x=radius, .y=falloff
	// Spot: .x=range, .y=falloff, .z=lightAngleScale .w=lightAngleOffset
	// Dir: .x=angularRadius
    // Sky: x=LOD
    float4 lightSpecific;
};

struct DirectionalLightShadowData
{
    float cascadeDistances[DIRECTIONAL_SHADOW_CASCADE_COUNT];
    float4x4 viewProjections[DIRECTIONAL_SHADOW_CASCADE_COUNT];
};

int GetLightBufferIndex(Buffer<int> lightIndexBuffer, uint tileCountX, int i, uint2 tileId)
{
    const uint index = tileId.y * tileCountX + tileId.x;
    const uint offset = index * MAX_LIGHTS_PER_TILE;

    return lightIndexBuffer.Load(offset + i);
}

int GetLightCount(Buffer<int> lightIndexBuffer, uint tileCountX, uint2 tileId)
{
    int result = 0;
    for (int i = 0; i < MAX_LIGHTS_PER_TILE; i++)
    {
        int lightIndex = GetLightBufferIndex(lightIndexBuffer, tileCountX, i, tileId);
        if (lightIndex == -1)
        {
            break;
        }

        result++;
    }

    return result;
}

Buffer<int> VisibleLightIndices;

int GetLightBufferIndex(int i, uint2 tileId)
{
    const uint index = tileId.y * View.tileCountX + tileId.x;
    const uint offset = index * MAX_LIGHTS_PER_TILE;

    return VisibleLightIndices[offset + i];
}