#include "Vertex.hlsli"
#include "Resources.hlsli"

vt::Tex2D<float3> Color;

struct Output
{
    [[vt::r11f_g11f_b10f]] float3 output : SV_Target0;
};

Output main(FullscreenTriangleVertex input)
{
    const float3 color = Color.Load(int3(input.position.xy, 0));

    Output output;
    output.output = color;

    return output;
}