#pragma once

#include "Common.hlsli"

enum DebugRenderingLayer
{
    Foreground,
    ForegroundWorld,
    World
};

struct DrawDebugMeshesPixelShaderInput
{
    float4 position : SV_Position;
    float3 worldPosition : POSITION;
    float4 tangent : TANGENT;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;

    float4 color : COLOR;
    uint primitiveIndex : PRIMITIVE_INDEX;
    uint objectId : OBJECTID;
    uint visProxyId : VISPROXYID;
    DebugRenderingLayer renderingLayer : RENDERINGLAYER;
};

Texture2D<float> SceneDepth;

void ApplyDebugRenderingLayerEffect(DebugRenderingLayer layer, float4 projectedPosition, inout float3 color)
{
    if (layer == DebugRenderingLayer::ForegroundWorld)
    {
        const float pixelDepth = SceneDepth.Load(int3(projectedPosition.xy, 0));

        const float depthDiff = (projectedPosition.z) - pixelDepth;

        // Currently inside an object
        if (depthDiff < -FLT_EPSILON)
        {
            const float lineWidth = 8.f;
            const float3 colorA = 0.1f;
            const float3 colorB = 0.25f;

            color *= uint(projectedPosition.x / lineWidth) % 2u ? colorA : colorB;
        }
    }
}