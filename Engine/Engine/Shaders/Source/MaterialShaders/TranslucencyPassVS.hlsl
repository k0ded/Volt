#include "RenderPipelineLegacy/Vertex.hlsli"
#include "ViewData.hlsli"
#include "Animation.hlsli"

#include "TranslucencyPassCommon.hlsli"
#include "Utility/Packing.hlsli"

TranslucenyPassPixelShaderInput MainVS(in FullVertex input)
{
    const PrimitiveDrawData primitiveData = GetPrimitiveDrawDataFromID(input.primitiveIndex);
    const GPUMesh gpuMesh = GetGPUMeshFromID(primitiveData.meshId);

    const float3 normal = UnpackNormalFromUInt32(input.normal);
    const float3 tangent = DecodeTangent(normal, input.tangent);

    float4x4 skinningMatrix = IDENTITY_MATRIX;
    if (primitiveData.isAnimated)
    {
        skinningMatrix = GetSkinningMatrix(primitiveData.boneOffset, input.influences, input.weights);
    }

    const float3 skinnedPosition = mul(skinningMatrix, float4(input.position, 1.f)).xyz;

    TransformedVertAttribs transformedVertAttribs = GetTransformedVertAttribs(primitiveData, gpuMesh, skinnedPosition, normal, tangent);

    TranslucenyPassPixelShaderInput result;
    result.position = mul(View.viewProjection, float4(transformedVertAttribs.position, 1.f));
    result.worldPosition = transformedVertAttribs.position;
    result.texCoords = input.texCoords;
    result.normal = normalize(transformedVertAttribs.normal);
    result.tangent = float4(normalize(transformedVertAttribs.tangent), input.tangentW);
    result.primitiveIndex = input.primitiveIndex;

    return result;
}