#include "Utility/FullscreenTriangleVertex.hlsli"

struct Output
{
    [[vt::rgba8]] float4 output : SV_Target0;
};

float4 Color;

Output MainPS(FullscreenTriangleVertex input)
{
    Output result;
    result.output = Color;

    return result;
}