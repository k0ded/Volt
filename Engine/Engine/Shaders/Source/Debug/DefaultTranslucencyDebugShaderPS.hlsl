#include "ViewData.hlsli"

#include "Debug/DrawDebugMeshesCommon.hlsli"
#include "MaterialShaders/TranslucencyPassCommon.hlsli"

#include "Lights/Lights.hlsli"
#include "PBR/PBR.hlsli"

struct TranslucenyDebugPassPixelShaderOutput
{
	float4 accumulation : SV_Target0;
	float revealage : SV_Target1;
	uint objectId : SV_Target2;
	uint visProxyId : SV_Target3;
};

TranslucenyDebugPassPixelShaderOutput MainPS(in DrawDebugMeshesPixelShaderInput input)
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

	TranslucenyDebugPassPixelShaderOutput result;
	result.accumulation = float4(outputColor * pbrInput.albedo.a, pbrInput.albedo.a) * weight;
	result.revealage = pbrInput.albedo.a;
	result.objectId = input.objectId;
	result.visProxyId = input.visProxyId;

	return result;
}