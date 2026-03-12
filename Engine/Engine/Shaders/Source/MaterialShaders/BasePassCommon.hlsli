#pragma once

struct BasePassPixelShaderInput
{
    float4 position : SV_Position;
    float4 tangent : TANGENT;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;

    uint primitiveIndex : PRIMITIVE_INDEX;
};

struct BasePassPixelShaderOutput
{
    float4 albedo : SV_Target0;
    float4 normal : SV_Target1;
    float2 material : SV_Target2;
    float3 emissive : SV_Target3;
};