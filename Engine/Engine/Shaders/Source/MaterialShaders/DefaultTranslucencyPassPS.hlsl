#include "ViewData.hlsli"
#include "MaterialShaders/TranslucencyPassCommon.hlsli"

#include "Lights/Lights.hlsli"
#include "PBR/PBR.hlsli"

TranslucenyPassPixelShaderOutput MainPS(in TranslucenyPassPixelShaderInput input)
{
	PBRInput pbrInput;
	pbrInput.albedo = float4(0.8f, 0.8f, 0.8f, 1.f);
	pbrInput.normal = input.normal;
	pbrInput.roughness = 0.8f;
	pbrInput.metallic = 0.f;
	pbrInput.emissive = 0.f;
	pbrInput.worldPosition = input.worldPosition;
	pbrInput.ao = 1.f;
	pbrInput.tileId = input.position.xy / LIGHT_CULLING_TILE_SIZE;

	const float3 outputColor = EvaluatePBR(pbrInput);
	const float weight = CalculateAccumulationWeight(pbrInput.albedo, input.position);

	TranslucenyPassPixelShaderOutput result;
	result.accumulation = float4(outputColor * pbrInput.albedo.a, pbrInput.albedo.a) * weight;
	result.revealage = pbrInput.albedo.a;

	return result;
}