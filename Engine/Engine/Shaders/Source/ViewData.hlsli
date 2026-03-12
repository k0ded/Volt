#pragma once

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
    float4x4 nonJitteredViewProjection;
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
    uint lightCount;

    uint frameIndex;
};

ConstantBuffer<ViewData> View;