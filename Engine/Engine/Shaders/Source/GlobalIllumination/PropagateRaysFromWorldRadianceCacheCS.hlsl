#include "GlobalIlluminationCommon.hlsli"
#include "SpatialHashTable.hlsli"

#include "Utility/Packing.hlsli"

#include "MonteCarlo.hlsli"
#include "Common.hlsli"

struct RayInfoData
{
	uint instanceId;
	uint hitT;
	uint packedBarycentrics;
	uint primitiveIndex;
	uint probeId;
	uint rayDirection;
};

RWTexture2D<float3> RWProbeAtlas;
RWTexture2D<float2> RWProbeVisibilityAtlas;
RWBuffer<uint> RWProbeStatus;

ByteAddressBuffer RayInfo;
StructuredBuffer<uint> WorldRadianceCacheCellCache;

uint IrradianceVolumeCascadeIndex;

float3 SkyColor(float3 direction)
{
	if (direction.y > 0.f)
	{
		return lerp(1.f, float3(0.25f, 0.5f, 1.f), direction.y);
	}
	else	
	{
		return 0.03f;
	}
}

groupshared uint2 GroupRadianceAndRayDirection[64];
groupshared float GroupHitT[64];

[numthreads(64, 1, 1)]
void PropagateRaysFromWorldRadianceCacheCS(uint DispatchThreadID : SV_DispatchThreadID, uint GroupThreadIndex : SV_GroupIndex)
{
	const RayInfoData rayInfo = RayInfo.Load<RayInfoData>(DispatchThreadID * sizeof(RayInfoData));
	
	const uint3 probeCoords = IrradianceVolume::GetProbeCoordsFromProbeIndex(rayInfo.probeId);
	const float3 probePosition = IrradianceVolume::GetProbeWorldPositionFromProbeCoords(probeCoords, IrradianceVolumeCascadeIndex);

	const float3 rayDirection = UnpackNormalFromUInt32(rayInfo.rayDirection);
	const float rayHitT = asfloat(rayInfo.hitT);

	uint radiance = 0;

	if (rayHitT > 0.f)
	{
		const float3 hitPosition = probePosition + rayDirection * rayHitT;
		
		SpatialHashTable worldRadianceCacheHashTable;

		uint hashIndex;
		if (worldRadianceCacheHashTable.Get(hitPosition, hashIndex))
		{
			radiance = WorldRadianceCacheCellCache[hashIndex];
		}
		else
		{
			radiance = 0; //PackRGBE(SkyColor(rayDirection));
		}
	}
	else
	{
		radiance = PackRGBE(SkyColor(rayDirection));
	}

	GroupHitT[GroupThreadIndex] = rayHitT * 0.01f;
	GroupRadianceAndRayDirection[GroupThreadIndex] = uint2(radiance, rayInfo.rayDirection);

	GroupMemoryBarrierWithGroupSync();
	
	const float2 rayUV = InverseEquiAreaSphericalMapping(rayDirection);
	const uint2 localTexelCoords = floor(rayUV * float(IrradianceVolumeProbeResolution));

	// Get the ray direction of the center of the texel
	const float3 texelRayDirection = EquiAreaSphericalMapping((localTexelCoords + 0.5f) / float(IrradianceVolumeProbeResolution));

	const float energyConservation = 0.95f;
	const float probeSpacing = IrradianceVolume::GetCascadeProbeSpacing(IrradianceVolumeCascadeIndex);

	float4 radianceResult = 0.f;
	float3 visibilityResult = 0.f;

	uint numBackfaceHits = 0;
	const uint numMaxBackfaceHitsToBlend = 6;
	
	bool isActive = true;

	for (uint i = 0; i < 64; ++i)
	{
		const uint2 radianceAndRayDirection = GroupRadianceAndRayDirection[i];
		float hitT = GroupHitT[i];

		if (hitT < 0.f)
		{	
			// Skip blending if too there were too many backface hits.
			if (numBackfaceHits++ > numMaxBackfaceHitsToBlend)
			{
				isActive = false;
				break;
			}

			continue;
		}

		const float3 rayDirection = UnpackNormalFromUInt32(radianceAndRayDirection.y);
		float weight = saturate(dot(texelRayDirection, rayDirection));
	
		weight = pow(weight, 2.5f);
		
		if (weight > FLT_EPSILON)
		{
			float3 radiance = UnpackRGBE(radianceAndRayDirection.x);
			radiance *= energyConservation;
			radianceResult += float4(radiance * weight, weight);

			visibilityResult += float3(hitT * weight, hitT * hitT * weight, weight);
		}
	}

	if (radianceResult.w > FLT_EPSILON)
	{
		radianceResult.xyz /= radianceResult.w;
	}

	if (visibilityResult.z > FLT_EPSILON)
	{
		visibilityResult.xy /= visibilityResult.z;
	} 

#if DEBUG
	radianceResult.rgb = (rayHitT > 0.f) ? float3(0.f, 1.f, 0.f) : float3(1.f, 0.f, 0.f);
#endif

	uint bitmaskIndex = rayInfo.probeId / 32u;
	uint bitIndex = rayInfo.probeId % 32u;

	// Reset bit.
	InterlockedAnd(RWProbeStatus[bitmaskIndex], ~(1u << bitIndex));

	const uint2 probeAtlasCoords = IrradianceVolume::GetProbeAtlasTexelCoordsFromProbeIndex(rayInfo.probeId, IrradianceVolumeCascadeIndex);

	if (isActive)
	{

		const float alpha = 0.1f;
		float3 prevRadiance = RWProbeAtlas[probeAtlasCoords + localTexelCoords + 1];
		RWProbeAtlas[probeAtlasCoords + localTexelCoords + 1] = alpha * radianceResult.xyz + (1.f - alpha) * prevRadiance;

		float2 prevVisibility = RWProbeVisibilityAtlas[probeAtlasCoords + localTexelCoords + 1];
		RWProbeVisibilityAtlas[probeAtlasCoords + localTexelCoords + 1] = alpha * visibilityResult.xy + (1.f - alpha) * prevVisibility;
	}
	else
	{
		RWProbeAtlas[probeAtlasCoords + localTexelCoords + 1] = 0.f;
 
		// Probe is inactive, set bit
		InterlockedOr(RWProbeStatus[bitmaskIndex], 1u << bitIndex);
	}
}