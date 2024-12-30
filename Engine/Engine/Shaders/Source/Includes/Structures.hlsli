#ifndef STRUCTURES_H
#define STRUCTURES_H

struct IndirectIndexedCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};

struct MeshTaskCommand
{
    uint drawId;
    uint taskCount;
    uint meshletOffset;
};

///// Rendering Structures /////
struct ViewData
{
    // Camera
    float4x4 view;
    float4x4 projection;
    float4x4 inverseView;
    float4x4 inverseProjection;
    float4x4 viewProjection;
    float4x4 inverseViewProjection;
    float4x4 prevViewProjection;
    float4 cameraPosition;
    float4 cullingFrustum;
    float2 depthUnpackConsts;
    float nearPlane;
    float farPlane;

    float2 currentFrameJitter;
    float2 prevFrameJitter;

    // Render Target
    uint2 renderSize;
    float2 invRenderSize;
    
    // Light Culling
    uint tileCountX;

    // Temp lights
    uint pointLightCount;
    uint spotLightCount;

    uint frameIndex;
};

#endif