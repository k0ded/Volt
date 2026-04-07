#include "UIDrawCommand.hlsli"
#include "UICommandCulling.hlsli"

StructuredBuffer<UICommand> R_Commands;

RWBuffer<uint> RW_PerTileCommandCount;

uint2 NumTiles;
uint NumCommands;

groupshared uint GroupNumCommands;

[numthreads(CULLING_GROUP_SIZE_X, CULLING_GROUP_SIZE_Y, 1)]
void CountUIElementsCS(uint2 DispatchThreadID : SV_DispatchThreadID, uint2 GroupID : SV_GroupID, uint GroupThreadIndex : SV_GroupIndex)
{
	if (GroupThreadIndex == 0)
	{
		GroupNumCommands = 0;
	}

    GroupMemoryBarrierWithGroupSync();

	const uint tileIndex = GroupID.y * NumTiles.x + GroupID.x;
	const uint2 minPixelCoords = GroupID * CULLING_GROUP_SIZE_X;
	const uint2 maxPixelCoords = (GroupID + 1) * CULLING_GROUP_SIZE_X;

	for (uint index = GroupThreadIndex; index < NumCommands; index += CULLING_GROUP_SIZE)
	{
		bool hasIntersectingCommand = false;

		if (index < NumCommands)
		{
			const UICommand command = R_Commands[index];
			hasIntersectingCommand = 
				IsAABBIntersectingAABB(command.bounds.xy, command.bounds.zw, minPixelCoords, maxPixelCoords) &&
				IsAABBIntersectingAABB(command.clipRect.xy, command.clipRect.zw, minPixelCoords, maxPixelCoords);
		}

		const uint numIntersectingCommands = WaveActiveCountBits(hasIntersectingCommand);

		uint offset;
		if (WaveIsFirstLane())
		{
			InterlockedAdd(GroupNumCommands, numIntersectingCommands, offset);
		}
		offset = WaveReadLaneFirst(offset) + WavePrefixCountBits(hasIntersectingCommand);
	}

    GroupMemoryBarrierWithGroupSync();

	if (GroupThreadIndex == 0)
	{
		RW_PerTileCommandCount[tileIndex] = GroupNumCommands;
	}
}