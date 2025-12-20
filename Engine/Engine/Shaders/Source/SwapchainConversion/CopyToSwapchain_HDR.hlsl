#include "Utility/FullscreenTriangleVertex.hlsli"

Texture2D<float4> SrcColor;

struct Output
{
    [[vt::rgb10_a2]] float4 output : SV_Target0;
};

float3 BT709_to_BT2020(float3 color) 
{
    float3x3 mat = float3x3
    (
        0.6274, 0.3293, 0.0433,
        0.0691, 0.9195, 0.0114,
        0.0164, 0.0880, 0.8956
    );
    return mul(mat, color);
}

float3 TonemapHDR(float3 x, float peakNits)
{
    return x * (peakNits / 10000.f);
}

float ST2084_Encode(float L)
{
    const float m1 = 2610.0 / 16384.0;
    const float m2 = 2523.0 / 32.0;
    const float c1 = 3424.0 / 4096.0;
    const float c2 = 2413.0 / 128.0;
    const float c3 = 2392.0 / 128.0;

    float Lm = pow(L, m1);
    return pow((c1 + c2 * Lm) / (1 + c3 * Lm), m2);
}

Output MainPS(FullscreenTriangleVertex input)
{
    float3 color = SrcColor.Load(int3(input.position.xy, 0)).rgb;

    color = BT709_to_BT2020(color);
    color = TonemapHDR(color, 500.f);

    Output output;
    output.output.r = ST2084_Encode(color.r);
    output.output.g = ST2084_Encode(color.g);
    output.output.b = ST2084_Encode(color.b);
    output.output.a = 0.f;

    return output;
}