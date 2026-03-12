#pragma once

Texture2D<float> BlueNoiseScalarTexture;
Texture2D<float4> BlueNoiseVec2Texture;
Texture2D<float4> BlueNoiseRGBATexture;
SamplerState BlueNoiseSampler;

uint3 BlueNoiseModuloMasks;
uint3 BlueNoiseDimensions;

// Spatiotemporal Blue Noise LuT based on "Spatiotemporal Blue Noise Masks" [Wolfe et al 2022] and 
// https://developer.nvidia.com/blog/rendering-in-real-time-with-spatiotemporal-blue-noise-textures-part-1/
float BlueNoiseScalar(uint2 pixelCoord, uint frameIndex)
{
    uint3 wrappedPixelCoord = uint3(pixelCoord, frameIndex) & BlueNoiseModuloMasks;
    uint3 texCoords = uint3(wrappedPixelCoord.x, wrappedPixelCoord.z * BlueNoiseDimensions.y + wrappedPixelCoord.y, 0);
    return BlueNoiseScalarTexture.Load(texCoords);
}

// Spatiotemporal Blue Noise LuT based on "Spatiotemporal Blue Noise Masks" [Wolfe et al 2022] and 
// https://developer.nvidia.com/blog/rendering-in-real-time-with-spatiotemporal-blue-noise-textures-part-1/
float2 BlueNoiseVec2(uint2 pixelCoord, uint frameIndex)
{
    uint3 wrappedPixelCoord = uint3(pixelCoord, frameIndex) & BlueNoiseModuloMasks;
    uint3 texCoords = uint3(wrappedPixelCoord.x, wrappedPixelCoord.z * BlueNoiseDimensions.y + wrappedPixelCoord.y, 0);
    return BlueNoiseVec2Texture.Load(texCoords).rg;
}

float4 BlueNoiseRGBA(uint2 pixelCoord, uint frameIndex)
{
    const float2 TextureSize = 512;

    pixelCoord += frameIndex;
    return BlueNoiseRGBATexture.SampleLevel(BlueNoiseSampler, float2(pixelCoord) / TextureSize, 0.f);
}