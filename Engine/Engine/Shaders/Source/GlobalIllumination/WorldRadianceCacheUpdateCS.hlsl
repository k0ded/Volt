#include "SpatialHashTable.hlsli"

RWStructuredBuffer<uint> RWWorldRadianceCacheCellCache;
RWStructuredBuffer<uint> RWWorldRadianceCacheCellInfo;

[numthreads(64, 1, 1)]
void WorldRadianceCacheUpdateCS(uint DispatchThreadID : SV_DispatchThreadID)
{
	if (DispatchThreadID >= SpatialHashTableSize)
	{
		return;
	}

	uint cellLifetime = RWWorldRadianceCacheCellInfo[DispatchThreadID];
	cellLifetime -= 1;

	// If the cell is "dead" we reset it and it's entry in the hash table. 
	if (cellLifetime == 0)
	{
		RWSpatialHashTableChecksum[DispatchThreadID] = 0;
		RWWorldRadianceCacheCellCache[DispatchThreadID] = 0;
	}

	RWWorldRadianceCacheCellInfo[DispatchThreadID] = cellLifetime;
}