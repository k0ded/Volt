RWTexture2D<float4> SourceMip;
RWTexture2D<float4> RWDstMip;

uint SrcWidth;
uint SrcHeight;
uint DstWidth;
uint DstHeight;

uint SrcMip;

[numthreads(8, 8, 1)]
void GenerateMipMapsCS(uint2 DispatchThreadID : SV_DispatchThreadID)
{
	if (DispatchThreadID.x >= DstWidth ||
		DispatchThreadID.y >= DstHeight)
	{
		return;
	}

	// Load 4 pixels.
	
	const uint2 baseSrcPixelCoord = DispatchThreadID * 2u;

	float numValidSamples = 1.f;
	float4 result = 0.f;
	
	result += SourceMip[baseSrcPixelCoord];

	if (baseSrcPixelCoord.y + 1 < SrcHeight)
	{
		result += SourceMip[baseSrcPixelCoord + uint2(0, 1)];
		numValidSamples += 1.f;
	}

	if (baseSrcPixelCoord.x + 1 < SrcWidth)
	{
		result += SourceMip[baseSrcPixelCoord + uint2(1, 0)];
		numValidSamples += 1.f;
	}

	if (baseSrcPixelCoord.x + 1 < SrcWidth && 
		baseSrcPixelCoord.y + 1 < SrcHeight)
	{
		result += SourceMip[baseSrcPixelCoord + uint2(1, 1)];
		numValidSamples += 1.f;
	}

	RWDstMip[DispatchThreadID] = result / numValidSamples;
}