#include "RayTracing/RayTracingResourceTable.hlsli"
#include "RayTracing/RayTracingTriangleAttributes.hlsli"

#include "Utility/Packing.hlsli"
#include "RenderScene/GPUScene.hlsli"

#include "GlobalIlluminationCommon.hlsli"

#include "PBR/BRDF.hlsli"
#include "PBR/LightEvaluation.hlsli"

struct RayInfoData
{
	uint instanceId;
	uint hitT;
	uint packedBarycentrics;
	uint primitiveIndex;
	uint probeId;
	uint rayDirection;
};

RWStructuredBuffer<uint> RWWorldRadianceCacheCellCache;

ByteAddressBuffer RayInfo;

StructuredBuffer<uint> WorldRadianceCacheCellsToShade;
StructuredBuffer<uint2> WorldRadianceCacheCellShadingInfo;

[numthreads(64, 1, 1)]
void WorldRadianceCacheShadeCellsCS(uint DispatchThreadID : SV_DispatchThreadID)
{
	const uint numCellsToShade = WorldRadianceCacheCellsToShade[0];
	if (DispatchThreadID >= numCellsToShade)
	{
		return;
	}

	const uint cellHashIndex = WorldRadianceCacheCellsToShade[DispatchThreadID + 1];
	const uint2 shadingInfo = WorldRadianceCacheCellShadingInfo[cellHashIndex];

	const RayInfoData rayInfo = RayInfo.Load<RayInfoData>(shadingInfo.x * sizeof(RayInfoData));

	const PrimitiveDrawData primitiveData = PrimitiveDrawDataBuffer[rayInfo.instanceId];
	const GPUMesh gpuMesh = GPUMeshes[primitiveData.meshId];

	Barycentrics barycentrics;
	barycentrics.Initialize(UnpackUnorm2x16(rayInfo.packedBarycentrics));

	TriangleAttributes triangleAttribs = LoadTriangleAttributes(gpuMesh, barycentrics, rayInfo.primitiveIndex);
	const float3 worldPosition = primitiveData.transform.GetWorldPosition(triangleAttribs.position);

	const float3 albedo = 0.8f;
	const float metallic = 0.f;
	const float roughness = 0.8f;

	BRDFInput brdfInput;
	brdfInput.V = UnpackNormalFromUInt32(rayInfo.rayDirection);
	brdfInput.N = triangleAttribs.normal;
	brdfInput.diffuseColor = CalculateDiffuseColor(albedo, metallic);
	brdfInput.f0 = CalculateF0(albedo, metallic);
	brdfInput.f90 = CalculateF90(albedo, metallic);
	brdfInput.roughness = roughness;
	brdfInput.metalness = metallic;

	float3 radiance = 0.f;

	// Evaluate lights
	for (uint i = 0; i < View.lightCount; ++i)
	{
		const LightDrawData light = SceneLights[i];
		if (light.lightType == SceneLightType::SLT_Point)
		{
		    radiance += EvaluatePointLight(light, brdfInput, worldPosition);
		}
		else if (light.lightType == SceneLightType::SLT_Spot)
		{
		    radiance += EvaluateSpotLight(light, brdfInput, worldPosition);
		}
		else if (light.lightType == SceneLightType::SLT_Directional)
		{
		    radiance += EvaluateDirectionalLight(light, brdfInput, worldPosition);
		}
		else if (light.lightType == SceneLightType::SLT_Sky)
		{
		    radiance += EvaluateIBL(brdfInput, light);
		}
	}

	RWWorldRadianceCacheCellCache[cellHashIndex] = PackRGBE(radiance);
}