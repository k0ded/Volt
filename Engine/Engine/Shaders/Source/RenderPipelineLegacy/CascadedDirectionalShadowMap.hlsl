#include "Vertex.hlsli"
#include "ViewData.hlsli"
#include "Animation.hlsli"

#include "Utility/ShadowMapping.hlsli"
#include "Utility/VertexShaderHelpers.hlsli"

struct ShadowVertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
    
    [[vt::inputIndex(2)]] uint4 influences : INFLUENCES;
    [[vt::inputIndex(2)]] float4 weights : WEIGHTS;

    [[vt::instance]] uint primitiveIndex : PRIMITIVEINDEX;
    uint instanceId : SV_InstanceID;
};

struct VSToPS
{
    float4 position : SV_Position;
    uint target : SV_RenderTargetArrayIndex;
};

uint CascadeIndex;

VSToPS MainVS(in ShadowVertex input)
{
    const PrimitiveDrawData primitiveData = GetPrimitiveDrawDataFromID(input.primitiveIndex);
    const GPUMesh gpuMesh = GetGPUMeshFromID(primitiveData.meshId);

    float4x4 skinningMatrix = IDENTITY_MATRIX;
    if (primitiveData.isAnimated)
    {
        skinningMatrix = GetSkinningMatrix(primitiveData.boneOffset, input.influences, input.weights);
    }

    const float3 skinnedPosition = mul(skinningMatrix, float4(input.position, 1.f)).xyz;

    VSToPS result;
    result.position = mul(CascadedDirectionalLightShadowMapping.viewProjections[CascadeIndex], float4(VertexShaderHelpers::TransformVertexToWorldSpace(primitiveData, gpuMesh, skinnedPosition), 1.f));
    result.target = CascadeIndex;

    return result;
}

struct ColorOutput
{
    [[vt::d32f]];
};

ColorOutput MainPS(VSToPS input)
{
    ColorOutput output;
    return output;
}