#include "RenderPipelineLegacy/Vertex.hlsli"
#include "ViewData.hlsli"
#include "Animation.hlsli"

#include "DepthPrePassCommon.hlsli"
#include "Utility/Packing.hlsli"
#include "Utility/VertexShaderHelpers.hlsli"

DepthPrePassPixelShaderInput MainVS(in FullVertex input)
{
    const PrimitiveDrawData primitiveData = GetPrimitiveDrawDataFromID(input.primitiveIndex);
    const PrimitiveDrawData prevPrimitiveData = GetPrevPrimitiveDrawDataFromID(input.primitiveIndex);

    float4x4 skinningMatrix = IDENTITY_MATRIX;
    if (primitiveData.isAnimated)
    {
        skinningMatrix = GetSkinningMatrix(primitiveData.boneOffset, input.influences, input.weights);
    }

    const float3 skinnedPosition = mul(skinningMatrix, float4(input.position, 1.f)).xyz;

    DepthPrePassPixelShaderInput result;
    result.position = mul(View.viewProjection, float4(VertexShaderHelpers::TransformVertexToWorldSpace(primitiveData, skinnedPosition), 1.f));
    result.prevPosition = mul(View.prevViewProjection, float4(VertexShaderHelpers::TransformVertexToWorldSpace(primitiveData, skinnedPosition), 1.f));
    result.currPosition = result.position;
    result.texCoords = input.texCoords;

    return result;
}
