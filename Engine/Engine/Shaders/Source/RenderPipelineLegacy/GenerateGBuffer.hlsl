#include "RenderScene/GPUScene.hlsli"
#include "Utility/Packing.hlsli"
#include "ViewData.hlsli"
#include "GBufferCommon.hlsli"

struct GBufferVertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
    
    [[vt::inputIndex(1)]] uint normal : NORMAL;
    [[vt::inputIndex(1)]] float tangent : TANGENT;
    [[vt::inputIndex(1)]] float tangentW : TANGENTW;
    [[vt::inputIndex(1)]] [[vt::half2]] float2 texCoords : TEXCOORD;

    uint instanceId : SV_InstanceID;    
};

Buffer<uint> PrimitiveDrawDataIndirection;

GBufferPixelShaderInput MainVS(in GBufferVertex input)
{
    const uint primitiveIndex = PrimitiveDrawDataIndirection[input.instanceId];
    const PrimitiveDrawData primitiveData = PrimitiveDrawDataBuffer[primitiveIndex];

    const float3 normal = DecodeNormal(input.normal);
    const float3 tangent = DecodeTangent(normal, input.tangent);

    GBufferPixelShaderInput result;
    result.position = mul(View.viewProjection, float4(primitiveData.transform.GetWorldPosition(input.position), 1.f));
    result.texCoords = input.texCoords;
    result.normal = normalize(primitiveData.transform.RotateVector(normal));
    result.tangent = float4(normalize(primitiveData.transform.RotateVector(tangent)), input.tangentW);
    result.primitiveIndex = primitiveIndex;

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