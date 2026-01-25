#include "Material/MaterialShader.hlsli"

#include "MaterialShaders/TranslucencyPassCommon.hlsli"
#include "Utility/Utility.hlsli"

#include "Lights/Lights.hlsli"
#include "PBR/PBR.hlsli"

TranslucenyPassPixelShaderOutput MainPS(in TranslucenyPassPixelShaderInput input)
{
    MaterialEvaluationData evaluationData;
    evaluationData.texCoords = input.texCoords;

    EvaluatedMaterial evaluatedMaterial = EvaluateMaterial(evaluationData);

    const float3x3 TBN = CalculateTBN(input.normal, input.tangent.xyz, input.tangent.w);
    const float3 resultNormal = normalize(mul(TBN, normalize(evaluatedMaterial.normal)));
    
	PBRInput pbrInput;
	pbrInput.albedo = evaluatedMaterial.albedo;
	pbrInput.normal = evaluatedMaterial.normal;
	pbrInput.roughness = evaluatedMaterial.roughness;
	pbrInput.metallic = evaluatedMaterial.metallic;
	pbrInput.emissive = evaluatedMaterial.emissive;
	pbrInput.worldPosition = input.worldPosition;
	pbrInput.ao = 0.f;
	pbrInput.tileId = input.position.xy / LIGHT_CULLING_TILE_SIZE;

	const float3 outputColor = EvaluatePBR(pbrInput);
	const float weight = CalculateAccumulationWeight(pbrInput.albedo, input.position);

	TranslucenyPassPixelShaderOutput result;
	result.accumulation = float4(outputColor * pbrInput.albedo.a, pbrInput.albedo.a) * weight;
	result.revealage = pbrInput.albedo.a;

    return result;
}