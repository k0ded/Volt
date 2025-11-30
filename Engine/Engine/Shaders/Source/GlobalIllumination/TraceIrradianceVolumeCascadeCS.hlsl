#include "RayTracing/RayTracingCommon.hlsli"
#include "RayTracing/RayTracingResourceTable.hlsli"
#include "RayTracing/RayTracingTriangleAttributes.hlsli"
#include "RayTracing/RayTracingInline.hlsli"

#include "Utility/Packing.hlsli"

#include "GlobalIlluminationCommon.hlsli"
#include "SpatialHashTable.hlsli"
#include "MonteCarlo.hlsli"
#include "BlueNoise.hlsli"

struct RayInfoData
{
	uint instanceId;
	uint hitT;
	uint packedBarycentrics;
	uint primitiveIndex;
	uint probeId;
	uint rayDirection;
};

RWByteAddressBuffer RWRayInfo;

RWStructuredBuffer<uint> RWWorldRadianceCacheCellsToShade;
RWStructuredBuffer<uint2> RWWorldRadianceCacheCellShadingInfo;
RWBuffer<uint> RWShadingIndirectArgs;
RWBuffer<uint> RWWorldRadianceCacheCellMark;

RaytracingAccelerationStructure TLAS;

uint IrradianceVolumeCascadeIndex;

// Random number generation using pcg32i_random_t, using inc = 1. Our random state is a uint.
uint StepRNG(uint rngState)
{
  return rngState * 747796405 + 1;
}

// Steps the RNG and returns a floating-point value between 0 and 1 inclusive.
float StepAndOutputRNGFloat(inout uint rngState)
{
  // Condensed version of pcg_output_rxs_m_xs_32_32, with simple conversion to floating-point [0,1].
  rngState  = StepRNG(rngState);
  uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
  word      = (word >> 22) ^ word;
  return float(word) / 4294967295.0f;
}

[numthreads(64, 1, 1)]
void TraceIrradianceVolumeCascadeCS(uint GroupID : SV_GroupID, uint GroupThreadIndex : SV_GroupIndex, uint DispatchThreadID : SV_DispatchThreadID)
{
	if (DispatchThreadID == 0)
	{
		RWShadingIndirectArgs[1] = 1;
		RWShadingIndirectArgs[2] = 1;
	}

	const uint3 probeCoords = IrradianceVolume::GetProbeCoordsFromProbeIndex(GroupID);
	const float3 probePosition = IrradianceVolume::GetProbeWorldPositionFromProbeCoords(probeCoords, IrradianceVolumeCascadeIndex);
	
	const uint2 threadLocalPixelCoords = uint2(
		GroupThreadIndex % IrradianceVolumeProbeResolution,
		GroupThreadIndex / IrradianceVolumeProbeResolution
	);

	uint randomSeed = uint(uint(1973) + DispatchThreadID * uint(9277) + View.frameIndex * uint(26699)) | uint(1);
	const float2 texelCenterOffset = float2(StepAndOutputRNGFloat(randomSeed), StepAndOutputRNGFloat(randomSeed));

	const float2 threadUV = (threadLocalPixelCoords + texelCenterOffset) / float(IrradianceVolumeProbeResolution);
	const float3 rayDirection = EquiAreaSphericalMapping(threadUV);

	RayDescription rayDesc;
	rayDesc.origin = probePosition;
	rayDesc.direction = rayDirection;
	rayDesc.tMin = 0.f;
	rayDesc.tMax = 100000.f;

	const uint rayFlags = RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES;
	const uint instanceMask = 0xFF;

	RayTraceInlineResult inlineTraceResult = TraceInlineRay(TLAS, rayFlags, instanceMask, rayDesc);

	/*
		We will store:
		- instance id
		- hit distance
		- barycentrics
		- primitive index (triangle)
		- probe id
		- ray direction
	*/

	RayInfoData rayInfo;
	rayInfo.instanceId = inlineTraceResult.GetInstanceID();
	rayInfo.hitT = asuint(inlineTraceResult.hitT);
	rayInfo.packedBarycentrics = PackUnorm2x16(inlineTraceResult.barycentrics.GetRaw());
	rayInfo.primitiveIndex = inlineTraceResult.GetPrimitiveIndex();
	rayInfo.probeId = GroupID;
	rayInfo.rayDirection = PackNormalToUInt32(rayDirection);

	RWRayInfo.Store<RayInfoData>(DispatchThreadID * sizeof(RayInfoData), rayInfo);

	if (inlineTraceResult.IsHit())
	{
		const float3 hitPosition = rayDesc.origin + rayDesc.direction * inlineTraceResult.hitT;

		SpatialHashTable worldRadianceCacheHashTable;

		uint hashIndex;
		if (worldRadianceCacheHashTable.Insert(hitPosition, hashIndex))
		{
			const uint bitmaskIndex = hashIndex / 32;
			const uint bitIndex = hashIndex % 32u;

			uint prevBitmask;
			InterlockedOr(RWWorldRadianceCacheCellMark[bitmaskIndex], 1u << bitIndex, prevBitmask);

			const bool shouldAddToList = (prevBitmask & (1u << bitIndex)) == 0;

			const uint numToAdd = WaveActiveCountBits(shouldAddToList);
			const uint lanePrefix = WavePrefixCountBits(shouldAddToList);

			uint storeOffset;
			if (WaveIsFirstLane())
			{
				InterlockedAdd(RWWorldRadianceCacheCellsToShade[0], numToAdd, storeOffset);
				InterlockedMax(RWShadingIndirectArgs[0], DivideRoundUp(storeOffset + numToAdd, 64u));
			}
			storeOffset = WaveReadLaneFirst(storeOffset) + lanePrefix + 1;
			
			RWWorldRadianceCacheCellsToShade[storeOffset] = hashIndex;
			// Maybe pack probe id (12 bits) + ray id (22 bits) together?
			RWWorldRadianceCacheCellShadingInfo[hashIndex] = uint2(DispatchThreadID, GroupID);
		}
	}
}  