#pragma once

#include "Structures.hlsli"
#include "BlueNoise.hlsli"
#include "Utility.hlsli"

struct VolumetricFogParams
{
    float temporalReprojectionJitterScale;
    float froxelNearPlane;
    float froxelFarPlane;
    float scatteringFactor;
    int3 froxelVolumeDimensions;
};

float2 GetUVFromFroxelCoord(float2 froxelCoord, uint width, uint height)
{
    return froxelCoord / float2(width, height);
}

// Exponential distribution as in https://advances.realtimerendering.com/s2016/Siggraph2016_idTech6.pdf
float SliceToExponentialDepth(float nearPlane, float farPlane, int slice, int numSlices)
{
    return nearPlane * pow(farPlane / nearPlane, (float(slice) + 0.5f) / float(numSlices));
}

float SliceToExponentialDepthJittered(float nearPlane, float farPlane, float jitter, int slice, int numSlices)
{
    return nearPlane * pow(farPlane / nearPlane, (float(slice) + 0.5f + jitter) / float(numSlices));
}

float LinearDepthToExponentialUVDepth(float nearPlane, float farPlane, float linearDepth, int numSlices)
{
    const float oneOverLogFOverN = 1.f / log2(farPlane / nearPlane);
    const float scale = numSlices * oneOverLogFOverN;
    const float bias = -(numSlices * log2(nearPlane) * oneOverLogFOverN);

    return max(log2(linearDepth) * scale + bias, 0.f) / float(numSlices);
}

float3 GetWorldPositionFromFroxelCoord(int3 froxelCoord, in VolumetricFogParams fogParams, in BlueNoiseData blueNoiseData, in ViewData viewData)
{
    float2 uv = GetUVFromFroxelCoord(froxelCoord.xy + 0.5f + viewData.currentFrameJitter * fogParams.temporalReprojectionJitterScale, fogParams.froxelVolumeDimensions.x, fogParams.froxelVolumeDimensions.y);

    float linearDepth = float(froxelCoord.z) / float(fogParams.froxelVolumeDimensions.z);
    float volumeJitter = BlueNoiseScalar(froxelCoord.xy, viewData.frameIndex, blueNoiseData);
    float exponentialDepth = SliceToExponentialDepthJittered(fogParams.froxelNearPlane, fogParams.froxelFarPlane, volumeJitter, froxelCoord.z, fogParams.froxelVolumeDimensions.z);

    float rawDepth = LinearDepthToDeviceDepth(exponentialDepth, fogParams.froxelNearPlane, fogParams.froxelFarPlane);
    return ReconstructWorldPosition(viewData, uv, rawDepth);
}

float3 GetWorldPositionFromFroxelCoordNoJitter(int3 froxelCoord, in VolumetricFogParams fogParams, in ViewData viewData)
{
    float2 uv = GetUVFromFroxelCoord(froxelCoord.xy + 0.5f, fogParams.froxelVolumeDimensions.x, fogParams.froxelVolumeDimensions.y);
    
    float linearDepth = float(froxelCoord.z) / float(fogParams.froxelVolumeDimensions.z);
    float exponentialDepth = SliceToExponentialDepth(fogParams.froxelNearPlane, fogParams.froxelFarPlane, froxelCoord.z, fogParams.froxelVolumeDimensions.z);
    
    float rawDepth = LinearDepthToDeviceDepth(exponentialDepth, fogParams.froxelNearPlane, fogParams.froxelFarPlane);
    return ReconstructWorldPosition(viewData, uv, rawDepth);
}

float3 ApplyVolumetricFog(float2 screenUV, float sceneDepth, float3 color, in ViewData viewData, in VolumetricFogParams fogParams, in vt::TextureSampler pointSampler, in vt::Tex3D<float4> integratedFogVolume)
{
    const float linearDepth = LinearizeDepth(sceneDepth, viewData);
    const float uvDepth = LinearDepthToExponentialUVDepth(fogParams.froxelNearPlane, fogParams.froxelFarPlane, linearDepth, fogParams.froxelVolumeDimensions.z);
    
    const float4 scatteringTransmittance = integratedFogVolume.SampleLevel(pointSampler, float3(screenUV, uvDepth), 0.f);

    color = color * scatteringTransmittance.a + scatteringTransmittance.rgb;
    return color;
}