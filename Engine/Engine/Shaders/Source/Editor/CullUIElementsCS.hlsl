#include "UIDrawCommand.hlsli"
#include "UICommandCulling.hlsli"

RWStructuredBuffer<int> RW_CulledUIElements;

StructuredBuffer<UICommand> R_Commands;
Buffer<uint> R_PerTileCommandOffset;

uint2 NumTiles;
uint NumCommands;

#define MAX_UI_COMMANDS_PER_TILE_SORTING 1024

groupshared uint GroupCommands[MAX_UI_COMMANDS_PER_TILE_SORTING];
groupshared uint GroupNumCommands;

[numthreads(CULLING_GROUP_SIZE_X, CULLING_GROUP_SIZE_Y, 1)]
void CullUIElementsCS(uint2 DispatchThreadID : SV_DispatchThreadID, uint2 GroupID : SV_GroupID, uint GroupThreadIndex : SV_GroupIndex)
{
	if (GroupThreadIndex == 0)
	{
		GroupNumCommands = 0;
	}

	for (uint index = GroupThreadIndex; index < MAX_UI_COMMANDS_PER_TILE_SORTING; index += CULLING_GROUP_SIZE)
	{
		GroupCommands[index] = 0xFFFFFFFF;
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

		if (hasIntersectingCommand)
		{
			GroupCommands[offset] = index;
		}
	}

    GroupMemoryBarrierWithGroupSync();

	// Sort UI commands. (Bitonic sort)
	for (uint k = 2; k <= MAX_UI_COMMANDS_PER_TILE_SORTING; k <<= 1)
	{
		for (uint j = k >> 1; j > 0; j >>= 1)
		{
			for (uint i = GroupThreadIndex; i < MAX_UI_COMMANDS_PER_TILE_SORTING; i += CULLING_GROUP_SIZE)
			{
				uint index2 = i ^ j;

				if (index2 > i)
				{
					uint val0 = GroupCommands[i];
					uint val1 = GroupCommands[index2];

					bool ascending = (i & k) == 0;
				
					if ((val0 > val1) == ascending)
					{
						GroupCommands[i] = val1;
						GroupCommands[index2] = val0;
					}
				}
			}

			GroupMemoryBarrierWithGroupSync();
		}
	}

	const uint tileOffset = R_PerTileCommandOffset[tileIndex];
	const uint numCommands = GroupNumCommands;

	for (uint index = GroupThreadIndex; index < numCommands; index += CULLING_GROUP_SIZE)
	{
		RW_CulledUIElements[tileOffset + index] = GroupCommands[index];
	}
}