#include "Utility/Utility.hlsli"

#include "Noise.hlsli"
#include "BlueNoise.hlsli"

#include "Utility/FullscreenTriangleVertex.hlsli"

Texture2D<float3> FinalColor;

float MiddleGray;
float WhitePoint;
uint FrameIndex;

struct Output
{
    [[vt::r11f_g11f_b10f]] float4 output : SV_Target0;
};

float3 ReinhardTonemap(float3 color)
{
    //return color / (color + 1.f);
    return color;
}

Output MainPS(FullscreenTriangleVertex input)
{
    float3 pixelColor = ReinhardTonemap(FinalColor.Load(int3(input.position.xy, 0)));

    float blueNoise = BlueNoiseScalar(input.position.xy, FrameIndex);

    pixelColor += (blueNoise - 0.5f) * (1.f / 256.f);

    Output output;
    output.output = float4(pixelColor, 1.f);
    return output;
}