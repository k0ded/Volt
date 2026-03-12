#include "FullscreenTriangleVertex.hlsli"

Texture2D<float4> Color;

float4 MainPS(FullscreenTriangleVertex input) : SV_Target0
{
    const float4 color = Color.Load(int3(input.position.xy, 0));
    return color;
}