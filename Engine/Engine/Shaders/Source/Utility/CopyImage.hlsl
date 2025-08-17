#include "FullscreenTriangleVertex.hlsli"

Texture2D<float4> Color;

struct Output
{
    [[vt::rgba8]] float4 output : SV_Target0;
};

Output MainPS(FullscreenTriangleVertex input)
{
    const float4 color = Color.Load(int3(input.position.xy, 0));

    Output output;
    output.output = color;

    return output;
}