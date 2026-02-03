#include "ViewData.hlsli"

#include "Debug/DrawDebugMeshesCommon.hlsli"
#include "Lights/Lights.hlsli"
#include "PBR/PBR.hlsli"

struct DrawDebugMeshesPixelShaderOutput
{
	[[vt::rgba8]] float4 color : SV_Target0;
	[[vt::r32ui]] uint objectId : SV_Target1;
    [[vt::d32f]];
};

DrawDebugMeshesPixelShaderOutput MainPS(in DrawDebugMeshesPixelShaderInput input)
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

	DrawDebugMeshesPixelShaderOutput result;
	result.color = float4(outputColor, pbrInput.albedo.a);
	result.objectId = input.objectId;

	return result;
}