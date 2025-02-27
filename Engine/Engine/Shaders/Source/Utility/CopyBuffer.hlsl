#include "Resources.hlsli"

struct Constants
{
    vt::RWTypedBuffer<uint> dstBuffer;
    vt::TypedBuffer<uint> srcBuffer;

    uint typeSizeInUINT;
    uint copyCount;
};

[numthreads(64, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();

    // Every thread copies one UINT
    uint copyIndex = dispatchThreadId / constants.typeSizeInUINT;

    if (copyIndex < constants.copyCount)
    {
        constants.dstBuffer.Store(dispatchThreadId, constants.srcBuffer.Load(dispatchThreadId));    
    }
}