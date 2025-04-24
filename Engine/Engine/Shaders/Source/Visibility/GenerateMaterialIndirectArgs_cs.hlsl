#include "Utility.hlsli"
#include "Resources.hlsli"

#define TG_SIZE 256

vt::TypedBuffer<uint> MaterialCounts;
vt::RWTypedBuffer<uint> RWIndirectArgsBuffer;
uint MaterialCount;

[numthreads(32, 1, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    if (threadId.x >= MaterialCount)
    {
        return;
    }
    
    const uint argsIndex = threadId.x * 3;
    const uint materialCount = MaterialCounts.Load(threadId.x);
    
    RWIndirectArgsBuffer.Store(argsIndex, materialCount > 0 ? DivideRoundUp(materialCount, TG_SIZE) : 0);
    RWIndirectArgsBuffer.Store(argsIndex + 1, 1);
    RWIndirectArgsBuffer.Store(argsIndex + 2, 1);
}