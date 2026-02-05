#include "Utility/Utility.hlsli"

#include "Noise.hlsli"
#include "BlueNoise.hlsli"

#include "Utility/FullscreenTriangleVertex.hlsli"

Texture2D<float3> FinalColor;

float MiddleGray;
float WhitePoint;
uint FrameIndex;
uint IsHDRMonitor;

float3 ReinhardTonemap(float3 color)
{
    return color / (color + 1.f);
    return color;
}

float3 MainPS(FullscreenTriangleVertex input) : SV_Target0
{
    float3 pixelColor = FinalColor.Load(int3(input.position.xy, 0));

    if (!IsHDRMonitor)
    {
        pixelColor = ReinhardTonemap(pixelColor);
    }

    float blueNoise = BlueNoiseScalar(input.position.xy, FrameIndex);

    pixelColor += (blueNoise - 0.5f) * (1.f / 256.f);
    return pixelColor;
}