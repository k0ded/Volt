#include "GlobalIlluminationCommon.hlsli"
#include "SpatialHashTable.hlsli"

#include "MonteCarlo.hlsli"
#include "Utility/Packing.hlsli"

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

[numthreads(64, 1, 1)]
void PropagateRaysFromWorldRadianceCacheCS(uint DispatchThreadID : SV_DispatchThreadID)
{
	const RayInfoData rayInfo = RayInfo.Load<RayInfoData>(DispatchThreadID * sizeof(RayInfoData));
	
	const uint3 probeCoords = IrradianceVolume::GetProbeCoordsFromProbeIndex(rayInfo.probeId);
	const float3 probePosition = IrradianceVolume::GetProbeWorldPositionFromProbeCoords(probeCoords, IrradianceVolumeCascadeIndex);
	const uint2 probeAtlasCoords = IrradianceVolume::GetProbeAtlasPixelCoordsFromProbeIndex(rayInfo.probeId, IrradianceVolumeCascadeIndex);

	const float3 rayDirection = UnpackNormalFromUInt32(rayInfo.rayDirection);
	const float hitT = asfloat(rayInfo.hitT);

	float3 radiance = 0.f;

	if (hitT > 0.f)
	{
		const float3 hitPosition = probePosition + rayDirection * asfloat(rayInfo.hitT);
		
		SpatialHashTable worldRadianceCacheHashTable;

		uint hashIndex;
		if (worldRadianceCacheHashTable.Get(hitPosition, hashIndex))
		{
			radiance = UnpackRGBE(WorldRadianceCacheCellCache[hashIndex]);
		}
	}
	else
	{
		radiance = SkyColor(rayDirection);
	}

	const float2 rayUV = InverseEquiAreaSphericalMapping(rayDirection);
	const uint2 localTexelCoords = rayUV * float(IrradianceVolumeProbeResolution);

	const float alpha = 0.5f;

	float3 prevRadiance = RWProbeAtlas[probeAtlasCoords + localTexelCoords + 1];
	RWProbeAtlas[probeAtlasCoords + localTexelCoords + 1] = alpha * radiance + (1.f - alpha) * prevRadiance;
}