#include "Defines.hlsli"
#include "Resources.hlsli"

vt::RWTypedBuffer<uint> DstBuffer;
vt::TypedBuffer<uint> SrcBuffer;
vt::TypedBuffer<uint> ScatterIndices;

uint TypeSizeInUINT;
uint CopyCount;

[numthreads(64, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)
{
    // Every thread copies one UINT
    uint scatterIndex = dispatchThreadId / TypeSizeInUINT;
    uint scatterOffset = dispatchThreadId - scatterIndex * TypeSizeInUINT;

    if (scatterIndex < CopyCount)
    {
        const uint dstIndex = ScatterIndices.Load(scatterIndex) * TypeSizeInUINT + scatterOffset;
        const uint srcIndex = dispatchThreadId;

        DstBuffer.Store(dstIndex, SrcBuffer.Load(srcIndex));    
    }
}