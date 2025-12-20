#include "Utility/FullscreenTriangleVertex.hlsli"
#include "Utility/Utility.hlsli"

Texture2D<float4> SrcColor;

struct Output
{
    [[vt::rgba8]] float4 output : SV_Target0;
};

Output MainPS(FullscreenTriangleVertex input)
{
    const float3 color = SrcColor.Load(int3(input.position.xy, 0)).rgb;

    Output output;
    output.output = float4(LinearToSRGB(color), 1.f);

    return output;
}