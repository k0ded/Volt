[[vk::image_format("r32ui")]]
RWBuffer<uint> RW_Data;

//uint NumMaxItems;

[numthreads(64, 1, 1)]
void ComputeShaderTestCS(uint DispatchThreadID : SV_DispatchThreadID)
{
	if (DispatchThreadID < 10000)
	{
		RW_Data[DispatchThreadID] = DispatchThreadID;
	}
}
