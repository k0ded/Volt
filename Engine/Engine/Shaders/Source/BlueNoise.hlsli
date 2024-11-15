#pragma once

struct BlueNoiseData
{
    vt::Tex2D<float> blueNoiseScalarTexture;
    vt::Tex2D<float4> blueNoiseRGBATexture;
    vt::TextureSampler pointWrapSampler;
    uint3 moduloMasks;
    uint3 dimensions;
};

// Spatiotemporal Blue Noise LuT based on "Spatiotemporal Blue Noise Masks" [Wolfe et al 2022] and 
// https://developer.nvidia.com/blog/rendering-in-real-time-with-spatiotemporal-blue-noise-textures-part-1/
float BlueNoiseScalar(uint2 pixelCoord, uint frameIndex, in BlueNoiseData blueNoiseData)
{
    uint3 wrappedPixelCoord = uint3(pixelCoord, frameIndex) & blueNoiseData.moduloMasks;
    uint3 texCoords = uint3(wrappedPixelCoord.x, wrappedPixelCoord.z * blueNoiseData.dimensions.y + wrappedPixelCoord.y, 0);
    return blueNoiseData.blueNoiseScalarTexture.Load(texCoords);
}

float4 BlueNoiseRGBA(uint2 pixelCoord, uint frameIndex, in BlueNoiseData blueNoiseData)
{
    const float2 TextureSize = 512;

    pixelCoord += frameIndex;
    return blueNoiseData.blueNoiseRGBATexture.SampleLevel(blueNoiseData.pointWrapSampler, float2(pixelCoord) / TextureSize, 0.f);
}