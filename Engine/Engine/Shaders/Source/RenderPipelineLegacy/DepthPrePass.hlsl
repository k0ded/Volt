#include "Vertex.hlsli"
#include "ViewData.hlsli"
#include "Animation.hlsli"

#include "Utility/VertexShaderHelpers.hlsli"

struct DepthVertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;

    [[vt::inputIndex(1)]] uint normal : NORMAL;
    [[vt::inputIndex(1)]] float tangent : TANGENT;
    [[vt::inputIndex(1)]] float tangentW : TANGENTW;
    [[vt::inputIndex(1)]] [[vt::half2]] float2 texCoords : TEXCOORD;
    
    [[vt::inputIndex(2)]] uint4 influences : INFLUENCES;
    [[vt::inputIndex(2)]] float4 weights : WEIGHTS;

    [[vt::instance]] uint primitiveIndex : PRIMITIVEINDEX;
    uint instanceId : SV_InstanceID;
};

struct VSToPS
{
    float4 position : SV_Position;
    float4 currPosition : CURR_POSITION;
    float4 prevPosition : PREV_POSITION;
    float2 texCoords : TEXCOORD;
};

VSToPS MainVS(in DepthVertex input)
{
    const PrimitiveDrawData primitiveData = GetPrimitiveDrawDataFromID(input.primitiveIndex);
    const PrimitiveDrawData prevPrimitiveData = GetPrevPrimitiveDrawDataFromID(input.primitiveIndex);
    const GPUMesh gpuMesh = GetGPUMeshFromID(primitiveData.meshId);

    float4x4 skinningMatrix = IDENTITY_MATRIX;
    if (primitiveData.isAnimated)
    {
        skinningMatrix = GetSkinningMatrix(primitiveData.boneOffset, input.influences, input.weights);
    }

    const float3 skinnedPosition = mul(skinningMatrix, float4(input.position, 1.f)).xyz;

    VSToPS result;
    result.position = mul(View.viewProjection, float4(VertexShaderHelpers::TransformVertexToWorldSpace(primitiveData, gpuMesh, skinnedPosition), 1.f));
    result.prevPosition = mul(View.prevViewProjection, float4(VertexShaderHelpers::TransformVertexToWorldSpace(primitiveData, gpuMesh, skinnedPosition), 1.f));
    result.currPosition = result.position;
    result.texCoords = input.texCoords;

    return result;
}

struct PSOutput
{
    [[vt::rg16f]] float2 velocity : SV_Target0;
    [[vt::d32f]];
};

PSOutput MainPS(in VSToPS input)
{
    float3 currentPosNDC = input.currPosition.xyz / input.currPosition.w;
    float3 previousPosNDC = input.prevPosition.xyz / input.prevPosition.w;

    PSOutput result;
    result.velocity = ((previousPosNDC.xy - View.prevFrameJitter) - (currentPosNDC.xy - View.currentFrameJitter)) * 0.5f;

    return result;
}