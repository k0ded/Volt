#include "RenderPipelineLegacy/GBufferCommon.hlsli"
#include "Utility/Utility.hlsli"

SamplerState TextureSamplerState;

$(TextureDeclarations)

struct MaterialEvaluationData
{
    float2 texCoords;
};

struct EvaluatedMaterial
{
    float4 albedo;
    float roughness;
    float metallic;
    float3 normal;
    float3 emissive;
    
    void Setup()
    {
        albedo = 1.f;
        roughness = 0.9f;
        metallic = 0.f;
        normal = float3(0.5f, 0.5f, 1.f);
        emissive = 0.f;
    }
};

EvaluatedMaterial EvaluateMaterial(in MaterialEvaluationData evalData)
{
    $(EvaluateMaterial)
}

GBufferPixelShaderOutput MainPS(in GBufferPixelShaderInput input)
{
    MaterialEvaluationData evaluationData;
    evaluationData.texCoords = input.texCoords;

    EvaluatedMaterial evaluatedMaterial = EvaluateMaterial(evaluationData);

    const float3x3 TBN = CalculateTBN(input.normal, input.tangent.xyz, input.tangent.w);

    float3 resultNormal = evaluatedMaterial.normal.xyz * 2.f - 1.f;
    resultNormal.z = sqrt(1.f - saturate(resultNormal.x * resultNormal.x + resultNormal.y * resultNormal.y));
    resultNormal = normalize(mul(TBN, normalize(resultNormal)));

    GBufferPixelShaderOutput result;
    result.albedo = evaluatedMaterial.albedo;
    result.normal = float4(resultNormal * 0.5f + 0.5f, 1.f);
    result.material = float2(evaluatedMaterial.roughness, evaluatedMaterial.metallic);
    result.emissive = evaluatedMaterial.emissive;

    return result;
}