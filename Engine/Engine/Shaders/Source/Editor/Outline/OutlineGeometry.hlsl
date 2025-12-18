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

VSToPS MainVS(in Vertex input)
{
    const PrimitiveDrawData primitiveData = GetPrimitiveDrawDataFromID(input.primitiveIndex);
    const GPUMesh gpuMesh = GetGPUMeshFromID(primitiveData.meshId);

    VSToPS result;
    result.position = mul(View.viewProjection, float4(VertexShaderHelpers::TransformVertexToWorldSpace(primitiveData, gpuMesh, input.position), 1.f));

    return result;
}

struct PSOutput
{
    [[vt::rgba8]] float4 color : SV_Target0;
    [[vt::d32f]];
};

PSOutput MainPS(VSToPS input)
{
    PSOutput output;
    output.color = 1.f;
    return output;
}