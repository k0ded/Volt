#include "GlobalIlluminationCommon.hlsli"

RWTexture2D<float3> RWProbeAtlas;
RWTexture2D<float2> RWProbeVisibilityAtlas;

uint IrradianceVolumeCascadeIndex;

[numthreads(32, 1, 1)]
void IrradianceVolumeFillProbeBordersCS(uint GroupThreadIndex : SV_GroupIndex, uint GroupID : SV_GroupID)
{
	const uint numTexels = (IrradianceVolumeResolution - 1) * 4;

	const uint2 baseTexelCoords = IrradianceVolume::GetProbeAtlasTexelCoordsFromProbeIndex(GroupID, IrradianceVolumeCascadeIndex);
	const uint realProbeResolution = IrradianceVolume::GetRealProbeResolution();

	for (uint index = WaveGetLaneIndex(); index < numTexels; index += WaveGetLaneCount())
	{
		const uint side = index / realProbeResolution;
		const uint offset = index % realProbeResolution;

		uint2 dstCoords;
		uint2 srcCoords;

		float3 color = 1.f;

		if (side == 0)
		{
			dstCoords = uint2(offset, 0);
			srcCoords = uint2(realProbeResolution - offset - 1, 1);
		}
		else if (side == 1)
		{
			dstCoords = uint2(offset, realProbeResolution - 1);
			srcCoords = uint2(realProbeResolution - offset - 1, realProbeResolution - 2);
		}
		else if (side == 2)
		{	
			dstCoords = uint2(0, offset);
			srcCoords = uint2(1, realProbeResolution - offset - 1);
		}
		else
		{
			dstCoords = uint2(realProbeResolution - 1, offset);
			srcCoords = uint2(realProbeResolution - 2, realProbeResolution - offset - 1);
		}

		if (all(dstCoords == 0)
			|| all(dstCoords == realProbeResolution - 1)
			|| (dstCoords.x == 0 && dstCoords.y == realProbeResolution - 1)
			|| (dstCoords.x == realProbeResolution - 1 && dstCoords.y == 0))
		{
			srcCoords.x = clamp(realProbeResolution - dstCoords.x - 1, 1, IrradianceVolumeProbeResolution);
			srcCoords.y = clamp(realProbeResolution - dstCoords.y - 1, 1, IrradianceVolumeProbeResolution);

			color = float3(1.f, 0.f, 0.f);
		}

		RWProbeAtlas[baseTexelCoords + dstCoords] = RWProbeAtlas[baseTexelCoords + srcCoords];
		RWProbeVisibilityAtlas[baseTexelCoords + dstCoords] = RWProbeVisibilityAtlas[baseTexelCoords + srcCoords];
	}
} 