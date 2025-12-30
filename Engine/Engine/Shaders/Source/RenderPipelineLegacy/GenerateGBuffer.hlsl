#include "ViewData.hlsli"
#include "GBufferCommon.hlsli"
#include "Animation.hlsli"

#include "Utility/VertexShaderHelpers.hlsli"
#include "Utility/Packing.hlsli"

struct GBufferVertex
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

struct TransformedVertAttribs
{
    float3 position;
    float3 normal;
    float3 tangent;
};

TransformedVertAttribs GetTransformedVertAttribs(PrimitiveDrawData primitiveData, GPUMesh gpuMesh, float3 skinnedPosition, float3 normal, float3 tangent)
{
    Transform combinedTransform = primitiveData.transform.Combine(gpuMesh.transform);

    TransformedVertAttribs result;
    result.position = combinedTransform.TransformPosition(skinnedPosition);
    result.normal = combinedTransform.RotateVector(normal);
    result.tangent = combinedTransform.RotateVector(tangent);

    return result;
}

GBufferPixelShaderInput MainVS(in GBufferVertex input)
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

    GBufferPixelShaderInput result;
    result.position = mul(View.viewProjection, float4(transformedVertAttribs.position, 1.f));
    result.texCoords = input.texCoords;
    result.normal = normalize(transformedVertAttribs.normal);
    result.tangent = float4(normalize(transformedVertAttribs.tangent), input.tangentW);
    result.primitiveIndex = input.primitiveIndex;

    return result;
}

GBufferPixelShaderOutput MainPS(in GBufferPixelShaderInput input)
{
    GBufferPixelShaderOutput result;
    result.albedo = float4(0.8f.xxx, 1.f);
    result.normal = float4(input.normal * 0.5f + 0.5f, 1.f);
    result.material = float2(0.8f, 0.f);
    result.emissive = 0.f;

    return result;
}