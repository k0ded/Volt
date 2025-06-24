#include "GPUScene.hlsli"
#include "Utility/Bitwise.hlsli"

RWBuffer<uint> RWValidPrimitiveDrawData;
StructuredBuffer<PrimitiveDrawData> PrimitiveDrawDataBuffer;

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
        InterlockedAdd(RWValidPrimitiveDrawData[0], 1, index);
        RWValidPrimitiveDrawData[index + 1] = dispatchThreadId;
    }
};