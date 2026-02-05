#include "Utility/FullscreenTriangleVertex.hlsli"
#include "Utility/Utility.hlsli"

Texture2D<float4> SrcColor;

float4 MainPS(FullscreenTriangleVertex input) : SV_Target0
{
    const float3 color = SrcColor.Load(int3(input.position.xy, 0)).rgb;
    return float4(LinearToSRGB(color), 1.f);
}