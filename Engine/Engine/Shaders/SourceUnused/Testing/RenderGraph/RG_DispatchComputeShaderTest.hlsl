#define GROUP_SIZE 32

RWStructuredBuffer<uint> OutputBuffer;
uint InitialValue;

[numthreads(GROUP_SIZE, 1, 1)]
void MainCS(uint threadId : SV_DispatchThreadID)
{ 
    OutputBuffer[threadId] = InitialValue + threadId;
} 