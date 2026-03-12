#include "SpatialHashTable.hlsli"
#include "Utility/Packing.hlsli"


VT_SPECIFY_FORMAT("rgba16f")
RWTexture2D<float4> RWSceneColor;

Texture2D<float> SceneDepth;
StructuredBuffer<uint> WorldRadianceCacheCellCache;

[numthreads(8, 8, 1)]
void VisualizeSpatialHashTableCS(uint2 DispatchThreadID : SV_DispatchThreadID)
{
	if (any(DispatchThreadID >= View.renderSize))
	{
		return;
	}

#if 0
	const float pixelDepth = SceneDepth.Load(int3(DispatchThreadID, 0));
	if (pixelDepth > 0.f)
	{
		const float3 worldPosition = ReconstructWorldPosition(DispatchThreadID, pixelDepth);
		
		SpatialHashTable hashTable;

		uint hashIndex;
		if (hashTable.Get(worldPosition, hashIndex))
		{
			RWSceneColor[DispatchThreadID] = float4(UnpackRGBE(WorldRadianceCacheCellCache[hashIndex]), 1.f); //float4(GetRandomColor(hashIndex), 1.f);
		}
		else
		{
			RWSceneColor[DispatchThreadID] = float4(0.f, 0.f, 0.f, 1.f);
		}
	}
#endif
}