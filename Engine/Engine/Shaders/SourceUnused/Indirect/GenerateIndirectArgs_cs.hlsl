#include "Resources.hlsli"
#include "Utility.hlsli"

vt::RWTypedBuffer<uint> RWIndirectArgs;
vt::TypedBuffer<uint> CountBuffer;

uint ThreadGroupSize;

[numthreads(1, 1, 1)]
void main()
{
    const uint count = CountBuffer.Load(0);
    RWIndirectArgs.Store(0, DivideRoundUp(count, ThreadGroupSize));
    RWIndirectArgs.Store(1, 1);
    RWIndirectArgs.Store(2, 1);
}