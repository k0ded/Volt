#include "ViewData.hlsli"

#include "Utility/VertexShaderHelpers.hlsli"

struct Vertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
    [[vt::instance]] uint primitiveIndex : PRIMITIVEINDEX;
    uint instanceId : SV_InstanceID;
};

struct VSToPS
{
    float4 position : SV_Position;
    uint objectId : OBJECT_ID;
};

VSToPS MainVS(in Vertex input)
{
    const PrimitiveDrawData primitiveData = GetPrimitiveDrawDataFromID(input.primitiveIndex);

    VSToPS result;
    result.position = mul(View.viewProjection, float4(VertexShaderHelpers::TransformVertexToWorldSpace(primitiveData, input.position), 1.f));
    result.objectId = primitiveData.entityId;

    return result;
}

uint MainPS(in VSToPS input) : SV_Target0
{
    return input.objectId;
}