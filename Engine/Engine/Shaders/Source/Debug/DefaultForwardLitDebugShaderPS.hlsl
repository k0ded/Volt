#include "ViewData.hlsli"

#include "Debug/DrawDebugMeshesCommon.hlsli"
#include "Lights/Lights.hlsli"
#include "PBR/PBR.hlsli"

struct DrawDebugMeshesPixelShaderOutput
{
	float4 color : SV_Target0;
	uint objectId : SV_Target1;
	uint visProxyId : SV_Target2;
};

DrawDebugMeshesPixelShaderOutput MainPS(in DrawDebugMeshesPixelShaderInput input)
{
	PBRInput pbrInput;
	pbrInput.albedo = input.color;
	pbrInput.normal = input.normal;
	pbrInput.roughness = 0.8f;
	pbrInput.metallic = 0.f;
	pbrInput.emissive = 0.f;
	pbrInput.worldPosition = input.worldPosition;
	pbrInput.ao = 1.f;
	pbrInput.tileId = input.position.xy / LIGHT_CULLING_TILE_SIZE;

	const float3 outputColor = EvaluatePBR(pbrInput);

	DrawDebugMeshesPixelShaderOutput result;
	result.color = float4(outputColor, pbrInput.albedo.a);
	result.objectId = input.objectId;
	result.visProxyId = input.visProxyId;

	return result;
}