#include "Resources.hlsli"

#include "GIBSCommon.hlsli"

struct Constants
{
    vt::RWTypedBuffer<int> rwSurfelAllocator;  

    uint numMaxSurfels;
    bool reset;
};

[numthreads(1, 1, 1)]
void InitializeSurfelAllocatorCS()
{
    const Constants constants = GetConstants<Constants>();

    if (constants.reset)
    {
        constants.rwSurfelAllocator.Store(SurfelAllocator::Counter, 0);
        constants.rwSurfelAllocator.Store(SurfelAllocator::StackPointer, constants.numMaxSurfels - 1);
    }
    else
    {
        if (constants.rwSurfelAllocator.Load(SurfelAllocator::StackPointer) < 0)
        {
            constants.rwSurfelAllocator.Store(SurfelAllocator::StackPointer, 0);
        }
    }

    constants.rwSurfelAllocator.Store(SurfelAllocator::ValidSurfelCounter, 0);
}