VT_SPECIFY_FORMAT("rgba16f")
RWTexture2D<float4> RWIndirectLight;

VT_SPECIFY_FORMAT("rgba16f")
RWTexture2D<float4> RWPrevFrameIndirectLight;

float AccumulationAlpha;

[numthreads(8, 8, 1)]
void TemporalAccumulationCS(uint2 DispatchThreadID : SV_DispatchThreadID)
{
	float3 prevIndirectLight = RWPrevFrameIndirectLight[DispatchThreadID].rgb;
	float3 indirectLight = RWIndirectLight[DispatchThreadID].rgb;

	float3 newIndirectLight = lerp(prevIndirectLight, indirectLight, 0.01f);
	RWIndirectLight[DispatchThreadID].rgb = indirectLight;//newIndirectLight;
	RWPrevFrameIndirectLight[DispatchThreadID].rgb = newIndirectLight;
}
