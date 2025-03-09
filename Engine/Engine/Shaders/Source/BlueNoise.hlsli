#pragma once

struct BlueNoiseData
{
    vt::Tex2D<float> blueNoiseScalarTexture;
    vt::Tex2D<float4> blueNoiseVec2Texture;
    vt::Tex2D<float4> blueNoiseRGBATexture;
    vt::TextureSampler pointWrapSampler;
    uint3 moduloMasks;
    uint3 dimensions;
};

BlueNoiseData BlueNoise;

// Spatiotemporal Blue Noise LuT based on "Spatiotemporal Blue Noise Masks" [Wolfe et al 2022] and 
// https://developer.nvidia.com/blog/rendering-in-real-time-with-spatiotemporal-blue-noise-textures-part-1/
float BlueNoiseScalar(uint2 pixelCoord, uint frameIndex)
{
    uint3 wrappedPixelCoord = uint3(pixelCoord, frameIndex) & BlueNoise.moduloMasks;
    uint3 texCoords = uint3(wrappedPixelCoord.x, wrappedPixelCoord.z * BlueNoise.dimensions.y + wrappedPixelCoord.y, 0);
    return BlueNoise.blueNoiseScalarTexture.Load(texCoords);
}

// Spatiotemporal Blue Noise LuT based on "Spatiotemporal Blue Noise Masks" [Wolfe et al 2022] and 
// https://developer.nvidia.com/blog/rendering-in-real-time-with-spatiotemporal-blue-noise-textures-part-1/
float2 BlueNoiseVec2(uint2 pixelCoord, uint frameIndex)
{
    uint3 wrappedPixelCoord = uint3(pixelCoord, frameIndex) & BlueNoise.moduloMasks;
    uint3 texCoords = uint3(wrappedPixelCoord.x, wrappedPixelCoord.z * BlueNoise.dimensions.y + wrappedPixelCoord.y, 0);
    return BlueNoise.blueNoiseVec2Texture.Load(texCoords).rg;
}

float4 BlueNoiseRGBA(uint2 pixelCoord, uint frameIndex)
{
    const float2 TextureSize = 512;

    pixelCoord += frameIndex;
    return BlueNoise.blueNoiseRGBATexture.SampleLevel(BlueNoise.pointWrapSampler, float2(pixelCoord) / TextureSize, 0.f);
}