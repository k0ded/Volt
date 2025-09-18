#include "ViewData.hlsli"

VT_SPECIFY_FORMAT("rgba16f")
RWTexture2D<float4> RWSceneColor;

Texture2D<float4> IndirectLight;

[numthreads(8, 8, 1)]
void CombineLightingCS(uint2 DispatchThreadID : SV_DispatchThreadID)
{
	if (any(DispatchThreadID >= View.renderSize))
	{
		return;
	}

	RWSceneColor[DispatchThreadID] += IndirectLight[DispatchThreadID];
}