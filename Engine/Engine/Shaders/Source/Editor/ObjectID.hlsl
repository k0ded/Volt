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
    const GPUMesh gpuMesh = GetGPUMeshFromID(primitiveData.meshId);

    VSToPS result;
    result.position = mul(View.viewProjection, float4(VertexShaderHelpers::TransformVertexToWorldSpace(primitiveData, gpuMesh, input.position), 1.f));
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