#include "ViewData.hlsli"

#include "Utility/VertexShaderHelpers.hlsli"

struct Vertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
    [[vt::instance]] uint primitiveIndex : PRIMITIVEINDEX;
};

struct VSToPS
{
    float4 position : SV_Position;
};

StructuredBuffer<uint> PrimitivesToDraw;

VSToPS MainVS(in Vertex input)
{
    const PrimitiveDrawData primitiveData = GetPrimitiveDrawDataFromID(input.primitiveIndex);

    const uint bitmaskIndex = input.primitiveIndex / 32u;
    const uint bitIndex = input.primitiveIndex % 32u;

    VSToPS result;

    if ((PrimitivesToDraw[bitmaskIndex] & (1u << bitIndex)) == 0)
    {
        result.position = 0.f;
        return result;        
    }

    result.position = mul(View.viewProjection, float4(VertexShaderHelpers::TransformVertexToWorldSpace(primitiveData, input.position), 1.f));

    return result;
}

float4 MainPS(VSToPS input) : SV_Target0
{
    return 1.f;
}