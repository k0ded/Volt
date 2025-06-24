#pragma once

struct GBufferPixelShaderInput
{
    float4 position : SV_Position;
    float4 tangent : TANGENT;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;

    uint primitiveIndex : PRIMITIVE_INDEX;
};

struct GBufferPixelShaderOutput
{
    [[vt::rgba8]] float4 albedo : SV_Target0;
    [[vt::rgba16]] float4 normal : SV_Target1;
    [[vt::rg8]] float2 material : SV_Target2;
    [[vt::d32f]];
};