#include "RenderScene/GPUScene.hlsli"
#include "Utility/Packing.hlsli"
#include "ViewData.hlsli"
#include "GBufferCommon.hlsli"
#include "Animation.hlsli"

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

GBufferPixelShaderInput MainVS(in GBufferVertex input)
{
    const PrimitiveDrawData primitiveData = PrimitiveDrawDataBuffer[input.primitiveIndex];

    const float3 normal = UnpackNormalFromUInt32(input.normal);
    const float3 tangent = DecodeTangent(normal, input.tangent);

    float4x4 skinningMatrix = IDENTITY_MATRIX;
    if (primitiveData.isAnimated)
    {
        skinningMatrix = GetSkinningMatrix(primitiveData.boneOffset, input.influences, input.weights);
    }

    const float3 skinnedPosition = mul(skinningMatrix, float4(input.position, 1.f)).xyz;

    GBufferPixelShaderInput result;
    result.position = mul(View.viewProjection, float4(primitiveData.transform.GetWorldPosition(skinnedPosition), 1.f));
    result.texCoords = input.texCoords;
    result.normal = normalize(primitiveData.transform.RotateVector(normal));
    result.tangent = float4(normalize(primitiveData.transform.RotateVector(tangent)), input.tangentW);
    result.primitiveIndex = input.primitiveIndex;

    return result;
}

GBufferPixelShaderOutput MainPS(in GBufferPixelShaderInput input)
{
    GBufferPixelShaderOutput result;
    result.albedo = float4(0.8f.xxx, 1.f);
    result.normal = float4(input.normal * 0.5f + 0.5f, 1.f);
    result.material = float2(0.8f, 0.f);

    return result;
}