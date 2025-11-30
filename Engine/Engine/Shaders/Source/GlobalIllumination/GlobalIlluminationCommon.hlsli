#pragma once

#include "Utility/Utility.hlsli"

#include "Hashing.hlsli"

float WorldRadianceCacheLodDistance;
float WorldRadianceCacheCellSize;

namespace WorldRadianceCache
{
	uint CalculateCascadeIndex(float distanceToCamera)
	{
		// From Fast As Hell idTech 8 GI, page 18.
		return uint(exp2(floor(log2(1.f + (distanceToCamera / WorldRadianceCacheLodDistance)))));
	}

	float GetCellSize(uint lod)
	{
		return WorldRadianceCacheCellSize * lod;
	}
	
	int3 GetQuantizedPosition(float3 position, uint lod)
	{
		const float cellSize = GetCellSize(lod);
		return int3(floor(position / cellSize));
	}

	uint CalculateHash(float3 position, uint lod)
	{
		const int3 quantizedPosition = GetQuantizedPosition(position, lod);
		const uint hash = HashUint(lod + HashUint(quantizedPosition.z + HashUint(quantizedPosition.y + HashUint(quantizedPosition.x))));

		return hash;
	}
}

struct IrradianceVolumeConstants
{
	static const uint32_t NumMaxCascades = 10;

	float4 cascadeMinCornerAndSpacing[NumMaxCascades];
};

ConstantBuffer<IrradianceVolumeConstants> IrradianceVolumeConstantData;

float IrradianceVolumeBaseSpacing;
uint IrradianceVolumeResolution;
uint IrradianceVolumeProbeResolution;
uint IrradianceVolumeProbeAtlasResolution;

namespace IrradianceVolume
{
	uint3 GetProbeCoordsFromProbeIndex(uint probeIndex)
	{
		return Get3DCoordFrom1DIndex(probeIndex, IrradianceVolumeResolution, IrradianceVolumeResolution);
	}

	float3 GetProbeWorldPositionFromProbeCoords(uint3 probeCoords, uint cascadeIndex)
	{
		const float4 minCornerAndSpacing = IrradianceVolumeConstantData.cascadeMinCornerAndSpacing[cascadeIndex];
		const float3 probePosition = minCornerAndSpacing.xyz + probeCoords * minCornerAndSpacing.w;
		return probePosition;
	}

	float GetCascadeProbeSpacing(uint cascadeIndex)
	{
		return IrradianceVolumeConstantData.cascadeMinCornerAndSpacing[cascadeIndex].w;
	}

	// The base coords of a probe, aka border is not accounted for.
	uint2 GetProbeAtlasPixelCoordsFromProbeIndex(uint probeIndex, uint cascadeIndex)
	{
		const uint borderSize = 2;
		const uint realResolution = IrradianceVolumeProbeResolution + borderSize; 
		const uint numProbesPerRow = IrradianceVolumeProbeAtlasResolution / realResolution;

		// Add cascade offset.
		probeIndex += cascadeIndex * (IrradianceVolumeResolution * IrradianceVolumeResolution * IrradianceVolumeResolution);

		const uint2 probeAtlasCoords = uint2(probeIndex % numProbesPerRow, probeIndex / numProbesPerRow) * realResolution;
		return probeAtlasCoords;
	}
} 