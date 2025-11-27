#pragma once

#include "GlobalIlluminationCommon.hlsli"
#include "ViewData.hlsli"

RWBuffer<uint> RWSpatialHashTableChecksum;

uint SpatialHashTableSize;

struct SpatialHashTable
{
	static const uint MaxIterations = 32;

	bool Insert(float3 worldPosition, out uint hashIndex)
	{
		const float distanceToCamera = length(worldPosition - View.cameraPosition.xyz);
		const uint lod = WorldRadianceCache::CalculateCascadeIndex(distanceToCamera);
	
		uint hash = WorldRadianceCache::CalculateHash(worldPosition, lod);
		uint checksum = 0;

		hashIndex = hash % SpatialHashTableSize;

		// Make sure we don't get stuck in case the hash table is full.
		uint iterationIndex = 0;
		while (iterationIndex++ < MaxIterations)
		{
			checksum = Shiftxor(hash);
			
			uint storedChecksum;
			InterlockedCompareExchange(RWSpatialHashTableChecksum[hashIndex], 0, checksum, storedChecksum);

			if (storedChecksum == 0 || storedChecksum == checksum)
			{
				break;
			}
			else
			{
				// Entry is occupied, rehash.
				hash = HashUint(hash);
				hashIndex = hash % SpatialHashTableSize;
			}
		}

		return iterationIndex < MaxIterations;
	}

	bool Get(float3 worldPosition, out uint hashIndex)
	{
		const float distanceToCamera = length(worldPosition - View.cameraPosition.xyz);
		const uint lod = WorldRadianceCache::CalculateCascadeIndex(distanceToCamera);
	
		uint hash = WorldRadianceCache::CalculateHash(worldPosition, lod);
		uint checksum = 0;

		hashIndex = hash % SpatialHashTableSize;

		// Make sure we don't get stuck in case the hash table is full.
		uint iterationIndex = 0;
		while (iterationIndex++ < MaxIterations)
		{
			checksum = Shiftxor(hash);
			
			// #Note_Ivar: Is this ok to do? Or do we need to to a InterlockedAdd(0)?
			uint storedChecksum = RWSpatialHashTableChecksum[hashIndex];

			if (storedChecksum == checksum)
			{
				return true;
			}
			else if (storedChecksum == 0)
			{
				return false;
			}
			else
			{
				// Entry is occupied, rehash.
				hash = HashUint(hash);
				hashIndex = hash % SpatialHashTableSize;
			}
		}

		return iterationIndex < MaxIterations;
	}
};	