#include "Resources.hlsli"
#include "Structures.hlsli"

vt::RWTypedBuffer<uint> CountCommandBuffer;
vt::RWTypedBuffer<MeshTaskCommand> TaskCommands;

[numthreads(32, 1, 1)]
void MainCS(uint groupThreadId : SV_GroupThreadID)
{
    uint commandCount = CountCommandBuffer.Load(0);
    
    if (groupThreadId == 0)
    {
        //constants.countCommandBuffer.Store(1, min(commandCount, 65535));
        //constants.countCommandBuffer.Store(2, 1);
        //constants.countCommandBuffer.Store(3, 1);
        CountCommandBuffer.Store(1, min((commandCount + 31) / 32, 65535));
        CountCommandBuffer.Store(2, 32);
        CountCommandBuffer.Store(3, 1);
    }

    uint boundary = (commandCount + 31) & ~31;
    
    MeshTaskCommand dummyCommand;
    
    if (commandCount + groupThreadId < boundary)
    {
        TaskCommands.Store(commandCount + groupThreadId, dummyCommand);
    }
}