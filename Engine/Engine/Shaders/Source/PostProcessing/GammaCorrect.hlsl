#include "Vertex.hlsli"
#include "Resources.hlsli"
#include "Utility.hlsli"

vt::Tex2D<float3> FinalColor;

struct Output
{
    [[vt::r11f_g11f_b10f]] float3 output : SV_Target0;
};

Output main(FullscreenTriangleVertex input)
{
    float3 pixelColor = FinalColor.Load(int3(input.position.xy, 0));

    Output output;
    output.output = LinearToSRGB(pixelColor);
    return output;
}