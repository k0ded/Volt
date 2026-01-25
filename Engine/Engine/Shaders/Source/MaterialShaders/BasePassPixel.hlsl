#include "Material/MaterialShader.hlsli"

#include "MaterialShaders/BasePassCommon.hlsli"
#include "Utility/Utility.hlsli"

BasePassPixelShaderOutput MainPS(in BasePassPixelShaderInput input)
{
    MaterialEvaluationData evaluationData;
    evaluationData.texCoords = input.texCoords;

    EvaluatedMaterial evaluatedMaterial = EvaluateMaterial(evaluationData);

    const float3x3 TBN = CalculateTBN(input.normal, input.tangent.xyz, input.tangent.w);
    const float3 resultNormal = normalize(mul(TBN, normalize(evaluatedMaterial.normal)));

    BasePassPixelShaderOutput result;
    result.albedo = evaluatedMaterial.albedo;
    result.normal = float4(resultNormal * 0.5f + 0.5f, 1.f);
    result.material = float2(evaluatedMaterial.roughness, evaluatedMaterial.metallic);
    result.emissive = evaluatedMaterial.emissive;

    EvaluateAlphaMask(result.albedo.a);

    return result;
}