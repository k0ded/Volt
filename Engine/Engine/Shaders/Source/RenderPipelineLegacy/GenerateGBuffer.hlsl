#include "Structures.hlsli"

#include "GPUScene.hlsli"
#include "Packing.hlsli"

struct GBufferVertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
    
    [[vt::inputIndex(1)]] uint normal : NORMAL;
    [[vt::inputIndex(1)]] float tangent : TANGENT;
    [[vt::inputIndex(1)]] float tangentW : TANGENTW;
    [[vt::inputIndex(1)]] [[vt::half2]] float2 texCoords : TEXCOORD;

    uint primtiveIndex : SV_InstanceID;    
};

struct VSToPS
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;
};

ConstantBuffer<ViewData> View;
StructuredBuffer<PrimitiveDrawData> PrimitiveDrawDataBuffer;

float3 UnpackNormal(uint normal)
{
    uint2 octIntNormal;
    octIntNormal.x = normal & 0xFF;
    octIntNormal.y = (normal >> 8) & 0xFF;

    float2 octNormal = 0.f;
    octNormal.x = float(octIntNormal.x) / 255.f;
    octNormal.y = float(octIntNormal.y) / 255.f;

    return normalize(OctNormalDecode(octNormal));
}

VSToPS MainVS(in GBufferVertex input)
{
    const PrimitiveDrawData primitiveData = PrimitiveDrawDataBuffer[input.primtiveIndex];

    const float3x3 cameraNormalRotation = (float3x3)View.view;

    VSToPS result;
    result.position = mul(View.viewProjection, float4(primitiveData.transform.GetWorldPosition(input.position), 1.f));
    result.texCoords = input.texCoords;
    result.normal = normalize(mul(cameraNormalRotation, primitiveData.transform.RotateVector(UnpackNormal(input.normal))));

    return result;
}

struct PSOutput
{
    [[vt::rgba8]] float4 albedo : SV_Target0;
    [[vt::rgba16]] float4 normal : SV_Target1;
    [[vt::rg8]] float2 material : SV_Target2;
    [[vt::d32f]];
};

PSOutput MainPS(in VSToPS input)
{
    PSOutput result;
    result.albedo = float4(0.8f.xxx, 1.f);
    result.normal = float4(input.normal * 0.5f + 0.5f, 1.f);
    result.material = float2(0.8f, 0.f);

    return result;
}