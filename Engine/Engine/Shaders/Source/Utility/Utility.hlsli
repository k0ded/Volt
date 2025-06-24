#pragma once

#include "ViewData.hlsli"

float3 GetRandomColor(uint _seed)
{
    uint hash = (_seed ^ 61) ^ (_seed >> 16);
    hash = hash + (hash << 3);
    hash = hash ^ (hash >> 4);
    hash = hash * 0x27d4eb2d;
    hash = hash ^ (hash >> 15);
    return float3(float(hash & 255),
                float((hash >> 8) & 255),
                float((hash >> 16) & 255)) / 255.0;
}

uint DivideRoundUp(uint dividend, uint divisor)
{
    return (dividend + divisor - 1) / divisor;
}

float4x4 ToFloat4x4(float4x3 transform)
{
    return float4x4(float4(transform[0], 0.f), float4(transform[1], 0.f), float4(transform[2], 0.f), float4(0.f, 0.f, 0.f, 1.f));
}

// Returns a linear depth value between near plane and far plane
float LinearizeDepth(const float screenDepth)
{
    float depthLinearizeMul = View.depthUnpackConsts.x;
    float depthLinearizeAdd = View.depthUnpackConsts.y;
    // Optimised version of "-cameraClipNear / (cameraClipFar - projDepth * (cameraClipFar - cameraClipNear)) * cameraClipFar"
    return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

// Returns a linear depth value between near plane and far plane in range [0...1]
float LinearizeDepth01(const float screenDepth)
{
    const float linearDepth = LinearizeDepth(screenDepth);
    return (linearDepth - View.nearPlane) / (View.farPlane - View.nearPlane);
}

// Converts from [near...far] to [0...1] in device space.
float LinearDepthToDeviceDepth(float depth, float nearPlane, float farPlane)
{
    return 1.f - ((nearPlane * farPlane) / (depth * (nearPlane - farPlane)) - farPlane / (nearPlane - farPlane));
}

float3x3 CalculateTBN(float3 inNormal, float3 inTangent, float tangentW)
{
    const float3 normal = normalize(inNormal);
    const float3 tangent = normalize(inTangent);
    const float3 binormal = normalize(cross(normal, tangent)) * tangentW;
    
    return transpose(float3x3(tangent, binormal, normal));
}

float3 ReconstructWorldPosition(float2 texCoords, float pixelDepth)
{
    float x = texCoords.x * 2.f - 1.f;
    float y = texCoords.y * 2.f - 1.f;
    
    const float4 projSpacePos = float4(x, y, pixelDepth, 1.f);
    float4 viewSpacePos = mul(View.inverseProjection, projSpacePos);
    
    viewSpacePos /= viewSpacePos.w;
    
    const float4 worldSpacePos = mul(View.inverseView, viewSpacePos);
    return worldSpacePos.xyz;
}

float Distance2(float3 A, float3 B)
{
	float3 diff = B - A;
	return dot(diff, diff);
}

//Frostbite accurate SRGB to linear conversion
float3 SRGBToLinear(in float3 color)
{
    float3 linearRGBLo = color / 12.92f;
    float3 linearRGBHi = pow((color + 0.055f) / 1.055f, 2.4f);
    float3 linearRGB = select((color <= 0.04045f), linearRGBLo, linearRGBHi);

    return linearRGB;
}

float3 LinearToSRGB(in float3 color)
{
    float3 sRGBLo = color * 12.92f;
    float3 sRGBHi = pow(abs(color), 1.f / 2.4f) * 1.055f - 0.055f;
    float3 sRGB = select((color <= 0.0031308f), sRGBLo, sRGBHi);

    return sRGB;
}

uint Get1DIndexFrom3DCoord(uint x, uint y, uint z, uint maxX, uint maxY)
{
	return (z * maxX * maxY) + (y * maxX) + x;
}

uint3 Get3DCoordFrom1DIndex(uint index, uint maxX, uint maxY)
{
	uint z = index / (maxX * maxY);
	index -= (z * maxX * maxY);
	uint y = index / maxX;
	uint x = index % maxX;

	return uint3(x, y, z);
}