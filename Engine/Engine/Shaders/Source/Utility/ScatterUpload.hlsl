RWStructuredBuffer<uint> RWDstBuffer;
StructuredBuffer<uint> SrcBuffer;
Buffer<uint> ScatterIndices;

uint TypeSizeInUINT;
uint CopyCount;

[numthreads(64, 1, 1)]
void MainCS(uint dispatchThreadId : SV_DispatchThreadID)
{
    // Every thread copies one UINT
    uint scatterIndex = dispatchThreadId / TypeSizeInUINT;
    uint scatterOffset = dispatchThreadId - scatterIndex * TypeSizeInUINT;

    if (scatterIndex < CopyCount)
    {
        const uint dstIndex = ScatterIndices[scatterIndex] * TypeSizeInUINT + scatterOffset;
        const uint srcIndex = dispatchThreadId;

        RWDstBuffer[dstIndex] = SrcBuffer[srcIndex];
    }
}