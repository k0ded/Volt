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
    [[vt::rgba8]] float4 albedo : SV_Target0;
    [[vt::rgba16]] float4 normal : SV_Target1;
    [[vt::rg8]] float2 material : SV_Target2;
    [[vt::r11f_g11f_b10f]] float3 emissive : SV_Target3;
    [[vt::d32f]];
};