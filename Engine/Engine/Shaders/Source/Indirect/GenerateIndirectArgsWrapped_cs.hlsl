#include "Resources.hlsli"
#include "ComputeUtilities.hlsli"

vt::RWTypedBuffer<uint> RWIndirectArgs;
vt::TypedBuffer <uint> CountBuffer;

uint GroupSize;

[numthreads(1, 1, 1)]
void main()
{
    uint count = CountBuffer.Load(0);
    const uint3 dispatchCount = GetGroupCountWrapped(count, GroupSize);
    
    RWIndirectArgs.Store(0, dispatchCount.x);
    RWIndirectArgs.Store(1, dispatchCount.y);
    RWIndirectArgs.Store(2, dispatchCount.z);
}