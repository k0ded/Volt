#include "RayTracing/RayTracingResourceTable.hlsli"
#include "RayTracing/RayTracingTriangleAttributes.hlsli"
#include "RayTracing/RayTracingInline.hlsli"

#include "Utility/Packing.hlsli"
#include "RenderScene/GPUScene.hlsli"

#include "GlobalIlluminationCommon.hlsli"
#include "IrradianceVolumeSampling.hlsli"

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
RWStructuredBuffer<uint> RWWorldRadianceCacheCellInfo;

ByteAddressBuffer RayInfo;

StructuredBuffer<uint> WorldRadianceCacheCellsToShade;
StructuredBuffer<uint2> WorldRadianceCacheCellShadingInfo;

RaytracingAccelerationStructure TLAS;

uint WorldRadianceCacheCellLifetime;

float TraceLightVisibility(in LightDrawData light, float3 origin)
{
	const float visibilityBias = 1.f;

	RayDescription rayDesc;
	rayDesc.origin = origin;
	rayDesc.tMin = visibilityBias;
	rayDesc.tMax = 10000.f;

	if (light.lightType == SceneLightType::SLT_Point ||
		light.lightType == SceneLightType::SLT_Spot)
	{
		rayDesc.direction = normalize(light.position - origin);
	}
	else if (light.lightType == SceneLightType::SLT_Directional)
	{
		rayDesc.direction = light.direction;
	}
	else
	{
		rayDesc.tMax = 0.f;
	}

	const uint rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;
	const uint instanceMask = 0xFF;

	RayTraceInlineResult inlineTraceResult = TraceInlineRay(TLAS, rayFlags, instanceMask, rayDesc);

	return inlineTraceResult.IsHit() ? 0.f : 1.f;
}

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
	ConvertTriangleAttributesToWorldSpace(triangleAttribs, primitiveData.transform);

	const float3 albedo = 0.8f;
	const float3 diffuse = LambertDiffuse(albedo);

	float3 radiance = 0.f;

	// Evaluate lights
	for (uint i = 0; i < View.lightCount; ++i)
	{
		const LightDrawData light = SceneLights[i];

		const float3 lightContribution = GetLightContribution(light, triangleAttribs.position, triangleAttribs.normal);
		const float visibility = TraceLightVisibility(light, triangleAttribs.position);

		radiance += lightContribution * visibility * diffuse;
	}

	radiance += SampleIrradiance(triangleAttribs.position, triangleAttribs.normal) * diffuse;

	RWWorldRadianceCacheCellInfo[cellHashIndex] = WorldRadianceCacheCellLifetime;
	RWWorldRadianceCacheCellCache[cellHashIndex] = PackRGBE(radiance);
}