#pragma once

struct TranslucenyPassPixelShaderInput
{
    float4 position : SV_Position;
    float3 worldPosition : POSITION;
    float4 tangent : TANGENT;
    float3 normal : NORMAL;
    float2 texCoords : TEXCOORD;

    uint primitiveIndex : PRIMITIVE_INDEX;
};

struct TranslucenyPassPixelShaderOutput
{
	float4 accumulation : SV_Target0;
	float revealage : SV_Target1;
};

float CalculateAccumulationWeight(float4 albedo, float4 projectedPosition)
{
    const float weight = clamp(pow(min(1.f, albedo.a * 10.f) + 0.01f, 3.f) * 1e8f *
                         pow(1.0 - projectedPosition.z * 0.9f, 3.f), 1e-2f, 3e3f);

    return weight;
}