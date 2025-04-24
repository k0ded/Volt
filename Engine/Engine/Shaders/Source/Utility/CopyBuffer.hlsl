#include "Resources.hlsli"

vt::RWTypedBuffer<uint> DstBuffer;
vt::TypedBuffer<uint> SrcBuffer;

uint TypeSizeInUINT;
uint CopyCount;

[numthreads(64, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)
{
    // Every thread copies one UINT
    uint copyIndex = dispatchThreadId / TypeSizeInUINT;

    if (copyIndex < CopyCount)
    {
        DstBuffer.Store(dispatchThreadId, SrcBuffer.Load(dispatchThreadId));    
    }
}