#include "Vertex.hlsli"
#include "Resources.hlsli"
#include "Utility.hlsli"

vt::Tex2D<float3> FinalColor;

float3 main(FullscreenTriangleVertex input) : SV_Target0
{
    float3 pixelColor = FinalColor.Load(int3(input.position.xy, 0));
    return LinearToSRGB(pixelColor);
}