#include "RenderPipelineLegacy/GBufferCommon.hlsli"
#include "Utility/Utility.hlsli"
#include "StaticSamplerStates.hlsli"
#include "MaterialCommon.hlsli"

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
    const float3 resultNormal = normalize(mul(TBN, normalize(evaluatedMaterial.normal)));

    GBufferPixelShaderOutput result;
    result.albedo = evaluatedMaterial.albedo;
    result.normal = float4(resultNormal * 0.5f + 0.5f, 1.f);
    result.material = float2(evaluatedMaterial.roughness, evaluatedMaterial.metallic);
    result.emissive = evaluatedMaterial.emissive;

#if MATERIAL_BLEND_MODE == MATERIAL_BLEND_MODE_ALPHA_MASKED
    result.albedo.a = step(0.5f, result.albedo.a);
    clip(result.albedo.a < 0.01f ? -1.f : 1.f);
#endif

    return result;
}