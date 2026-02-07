#include "Material/MaterialShader.hlsli"

#include "Debug/DrawDebugMeshesCommon.hlsli"
#include "Utility/Utility.hlsli"

#include "Lights/Lights.hlsli"
#include "PBR/PBR.hlsli"

struct DrawDebugMeshesPixelShaderOutput
{
	[[vt::rgba8]] float4 color : SV_Target0;
	[[vt::r32ui]] uint objectId : SV_Target1;
	[[vt::r32ui]] uint visProxyId : SV_Target2;
    [[vt::d32f]];
};

DrawDebugMeshesPixelShaderOutput MainPS(in DrawDebugMeshesPixelShaderInput input, bool isFrontFace : SV_IsFrontFace)
{
    MaterialEvaluationData evaluationData;
    evaluationData.texCoords = input.texCoords;

    EvaluatedMaterial evaluatedMaterial = EvaluateMaterial(evaluationData);

    const float3x3 TBN = CalculateTBN(input.normal, input.tangent.xyz, input.tangent.w);
    float3 resultNormal = normalize(mul(TBN, normalize(evaluatedMaterial.normal)));
    
	EvaluateDoubleSided(resultNormal, isFrontFace);

	PBRInput pbrInput;
	pbrInput.albedo = evaluatedMaterial.albedo * input.color;
	pbrInput.normal = resultNormal;
	pbrInput.roughness = evaluatedMaterial.roughness;
	pbrInput.metallic = evaluatedMaterial.metallic;
	pbrInput.emissive = evaluatedMaterial.emissive;
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