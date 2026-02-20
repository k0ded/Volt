Texture2D<float4> SourceMip;
RWTexture2D<float4> RWDstMip;

uint SrcWidth;
uint SrcHeight;
uint DstWidth;
uint DstHeight;

uint SrcMip;

[numthreads(8, 8, 1)]
void GenerateMipMapsCS(uint2 DispatchThreadID : SV_DispatchThreadID)
{
	if (DstWidth >= DispatchThreadID.x &&
		DstHeight >= DispatchThreadID.y)
	{
		return;
	}

	// Load 4 pixels.
	
	float numValidSamples = 1.f;
	float4 result = 0.f;
	
	result += SourceMip.Load(int3(DispatchThreadID, SrcMip));

	if (DispatchThreadID.y + 1 < SrcHeight)
	{
		result += SourceMip.Load(int3(DispatchThreadID + uint2(0, 1), SrcMip));
		numValidSamples += 1.f;
	}

	if (DispatchThreadID.x + 1 < SrcWidth)
	{
		result += SourceMip.Load(int3(DispatchThreadID + uint2(1, 0), SrcMip));
		numValidSamples += 1.f;
	}

	if (DispatchThreadID.x + 1 < SrcWidth && 
		DispatchThreadID.y + 1 < SrcHeight)
	{
		result += SourceMip.Load(int3(DispatchThreadID + uint2(1, 1), SrcMip));
		numValidSamples += 1.f;
	}

	RWDstMip[DispatchThreadID] = result / numValidSamples;
}