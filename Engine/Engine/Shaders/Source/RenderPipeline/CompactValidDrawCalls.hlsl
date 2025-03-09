#include "Resources.hlsli"
#include "GPUScene.hlsli"
#include "Bitwise.hlsli"

#include "Atomics.hlsli"

vt::RWTypedBuffer<uint> RWValidPrimitiveDrawData;
vt::TypedBuffer<PrimitiveDrawData> PrimitiveDrawDataBuffer;

uint PrimitiveDrawDataCount;

[numthreads(64, 1, 1)]
void MainCS(uint dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId >= PrimitiveDrawDataCount)
    {
        return;
    }

    const PrimitiveDrawData primitiveDrawData = PrimitiveDrawDataBuffer.Load(dispatchThreadId);
    
    if (IsBitSet(primitiveDrawData.flags, PrimitiveFlags::Valid))
    {
        uint index;
        vt::InterlockedAdd(RWValidPrimitiveDrawData, 0, 1, index);
        RWValidPrimitiveDrawData.Store(index + 1, dispatchThreadId);
    }
};