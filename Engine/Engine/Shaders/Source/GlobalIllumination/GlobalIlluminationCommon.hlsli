#pragma once

#include "Utility/Utility.hlsli"

#include "Hashing.hlsli"
#include "MonteCarlo.hlsli"

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

	int4 cascadeScrollOffset[NumMaxCascades];
	float4 cascadeMinCornerAndSpacing[NumMaxCascades];
};

ConstantBuffer<IrradianceVolumeConstants> IrradianceVolumeConstantData;

float IrradianceVolumeBaseSpacing;
uint IrradianceVolumeResolution;
uint IrradianceVolumeProbeResolution;
uint IrradianceVolumeProbeAtlasResolution;
uint IrradianceVolumeNumCascades;

namespace IrradianceVolume
{
	uint3 GetProbeCoordsFromProbeIndex(uint probeIndex)
	{
		return Get3DCoordFrom1DIndex(probeIndex, IrradianceVolumeResolution, IrradianceVolumeResolution);
	}

	uint GetProbeIndexFromProbeCoords(uint3 probeCoords)
	{
		return Get1DIndexFrom3DCoord(probeCoords.x, probeCoords.y, probeCoords.z, IrradianceVolumeResolution, IrradianceVolumeResolution);
	}

	uint4 GetProbeCoordsAndCascadeIndexFromWorldPosition(float3 worldPosition)
	{
		for (uint i = 0; i < IrradianceVolumeNumCascades; ++i)
		{
			const float4 cascadeMinCornerAndSpacing = IrradianceVolumeConstantData.cascadeMinCornerAndSpacing[i];
			const float3 maxLocalPos = cascadeMinCornerAndSpacing.w * IrradianceVolumeResolution;
		
			const float3 cascadeLocalPosition = worldPosition - cascadeMinCornerAndSpacing.xyz;

			if (all(cascadeLocalPosition > 0.f) && all(cascadeLocalPosition < maxLocalPos))
			{
				uint3 probeCoord = floor(cascadeLocalPosition / cascadeMinCornerAndSpacing.w);
			
				return uint4(probeCoord, i);
			}
		}

		return 0u;
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

	uint GetRealProbeResolution()
	{
		const uint borderSize = 2;
		return IrradianceVolumeProbeResolution + borderSize;
	}

	// The base coords of a probe, aka border is not accounted for.
	uint2 GetProbeAtlasTexelCoordsFromProbeIndex(uint probeIndex, uint cascadeIndex)
	{
		const uint realResolution = GetRealProbeResolution();
		const uint numProbesPerRow = IrradianceVolumeProbeAtlasResolution / realResolution;

		// Add cascade offset.
		probeIndex += cascadeIndex * (IrradianceVolumeResolution * IrradianceVolumeResolution * IrradianceVolumeResolution);

		const uint2 probeAtlasCoords = uint2(probeIndex % numProbesPerRow, probeIndex / numProbesPerRow) * realResolution;
		return probeAtlasCoords;
	}

	float2 GetProbeSamplingUVFromNormal(uint probeIndex, uint cascadeIndex, float3 normal)
	{
		const uint2 baseTexelCoords = GetProbeAtlasTexelCoordsFromProbeIndex(probeIndex, cascadeIndex);

		const float2 localUv = InverseEquiAreaSphericalMapping(normal);
		const uint2 localTexelCoords = localUv * float(IrradianceVolumeProbeResolution);

		const float2 uv = (float2(baseTexelCoords + localTexelCoords) + 0.5f) / float(IrradianceVolumeProbeAtlasResolution);
		return uv;
	}
} 