#include "Resources.hlsli"
#include "Utility.hlsli"

#include "GIBSCommon.hlsli"

struct Constants
{
    vt::RWTypedBuffer<SurfelGridCell> surfelGrid;
    vt::TypedBuffer<Surfel> surfels;
    vt::TypedBuffer<uint> surfelAllocator;
};

[numthreads(64, 1, 1)]
void UpdateSurfels(uint dispatchThreadID : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();
    uint surfelCount = constants.surfelAllocator.Load(0);

    if (dispatchThreadID >= surfelCount)
    {
        return;
    }

    
}