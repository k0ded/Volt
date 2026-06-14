Texture2DArray<float2> HBAODeinterleavedAO;
RWTexture2D<float2> ReinterleavedAO;

[shader("compute")]
[numthreads(8, 8, 1)]
void MainCS(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    uint x, y, z;
    ReinterleavedAO.GetDimensions(x, y);

    if (dispatchThreadID.x >= x || dispatchThreadID.y >= y)
        return;

    int2 FullResPos = dispatchThreadID;
    int2 Offset = FullResPos & 3;
    int SliceId = Offset.y * 4 + Offset.x;
    int2 QuarterResPos = FullResPos >> 2;

    ReinterleavedAO[FullResPos] = HBAODeinterleavedAO[uint3(QuarterResPos, SliceId)];
}