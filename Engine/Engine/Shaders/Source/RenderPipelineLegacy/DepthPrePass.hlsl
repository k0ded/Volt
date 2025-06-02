#include "Vertex.hlsli"
#include "Structures.hlsli"

struct VSToPS
{
    float4 position : SV_Position;
};

ConstantBuffer<ViewData> View;

VSToPS MainVS(in Vertex input)
{
    VSToPS result;
    result.position = mul(View.viewProjection, float4(input.position, 1.f));

    return result;
}

struct PSOutput
{
    [[vt::rgba8]] float4 color : SV_Target0;
    [[vt::d32f]];
};

PSOutput MainPS(in VSToPS input)
{
    PSOutput result;
    result.color = 1.f;

    return result;
}