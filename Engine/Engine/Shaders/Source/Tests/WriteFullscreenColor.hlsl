#include "Utility/FullscreenTriangleVertex.hlsli"

float4 Color;

float4 MainPS(FullscreenTriangleVertex input) : SV_Target0
{
    return Color;
}