#include "Vertex.hlsli"
#include "Resources.hlsli"
#include "Utility.hlsli"

struct Constants
{
    vt::Tex2D<float3> finalColor;
};

struct Output
{
    [[vt::r11f_g11f_b10f]] float3 output : SV_Target0;
};

Output main(FullscreenTriangleVertex input)
{
    const Constants constants = GetConstants<Constants>();
    float3 pixelColor = constants.finalColor.Load(int3(input.position.xy, 0));

    Output output;
    output.output = LinearToSRGB(pixelColor);
    return output;
}