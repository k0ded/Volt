#include "RenderScene/GPUScene.hlsli"
#include "ViewData.hlsli"

struct Vertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
    uint instanceId : SV_InstanceID;
};

struct VSToPS
{
    float4 position : SV_Position;
    uint objectId : OBJECT_ID;
};

Buffer<uint> PrimitiveDrawDataIndirection;

VSToPS MainVS(in Vertex input)
{
    const PrimitiveDrawData primitiveData = PrimitiveDrawDataBuffer[PrimitiveDrawDataIndirection[input.instanceId]];

    VSToPS result;
    result.position = mul(View.viewProjection, float4(primitiveData.transform.GetWorldPosition(input.position), 1.f));
    result.objectId = primitiveData.entityId;

    return result;
}

struct PSOutput
{
    [[vt::r32ui]] uint objectId : SV_Target0;
    [[vt::d32f]];
};

PSOutput MainPS(in VSToPS input)
{
    PSOutput result;
    result.objectId = input.objectId;

    return result;
}