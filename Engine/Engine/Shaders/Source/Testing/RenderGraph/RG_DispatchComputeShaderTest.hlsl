#include "Resources.hlsli"

#define GROUP_SIZE 32

vt::RWTypedBuffer<uint> OutputBuffer;
uint InitialValue;

[numthreads(GROUP_SIZE, 1, 1)]
void main(uint threadId : SV_DispatchThreadID)
{ 
    OutputBuffer.Store(threadId, InitialValue + threadId);
} 