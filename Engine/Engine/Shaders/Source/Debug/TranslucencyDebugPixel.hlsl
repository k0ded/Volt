#include "Material/MaterialShader.hlsli"

#include "Debug/DrawDebugMeshesCommon.hlsli"
#include "MaterialShaders/TranslucencyPassCommon.hlsli"
#include "Utility/Utility.hlsli"

#include "Lights/Lights.hlsli"
#include "PBR/PBR.hlsli"

struct TranslucenyDebugPassPixelShaderOutput
{
	float4 accumulation : SV_Target0;
	float revealage : SV_Target1;
	uint objectId : SV_Target2;
	uint visProxyId : SV_Target3;
};

TranslucenyDebugPassPixelShaderOutput MainPS(in DrawDebugMeshesPixelShaderInput input, bool isFrontFace : SV_IsFrontFace)
{
    MaterialEvaluationData evaluationData;
    evaluationData.texCoords = input.texCoords;

    EvaluatedMaterial evaluatedMaterial = EvaluateMaterial(evaluationData);

    const float3x3 TBN = CalculateTBN(input.normal, input.tangent.xyz, input.tangent.w);
    float3 resultNormal = normalize(mul(TBN, normalize(evaluatedMaterial.normal)));
    
	EvaluateDoubleSided(resultNormal, isFrontFace);

	PBRInput pbrInput;
	pbrInput.albedo = evaluatedMaterial.albedo;
	pbrInput.normal = resultNormal;
	pbrInput.roughness = evaluatedMaterial.roughness;
	pbrInput.metallic = evaluatedMaterial.metallic;
	pbrInput.emissive = evaluatedMaterial.emissive;
	pbrInput.worldPosition = input.worldPosition;
	pbrInput.ao = 0.f;
	pbrInput.tileId = input.position.xy / LIGHT_CULLING_TILE_SIZE;

	const float3 outputColor = EvaluatePBR(pbrInput);
	const float weight = CalculateAccumulationWeight(pbrInput.albedo, input.position);

	TranslucenyDebugPassPixelShaderOutput result;
	result.accumulation = float4(outputColor * pbrInput.albedo.a, pbrInput.albedo.a) * weight;
	result.revealage = pbrInput.albedo.a;
	result.objectId = input.objectId;
	result.visProxyId = input.visProxyId;

    return result;
}