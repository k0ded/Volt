#include "Resources.hlsli"
#include "Atomics.hlsli"

#include "GIBSCommon.hlsli"

struct Constants
{
    vt::RWTypedBuffer<uint> surfelCellCounts;

    vt::TypedBuffer<Surfel> surfels;
    vt::TypedBuffer<uint> surfelAllocator;
};

[numthreads(64, 1, 1)]
void GenerateSurfelCellCounts(uint dispatchThreadID : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();
    uint surfelCount = constants.surfelAllocator.Load(0);

    if (dispatchThreadID >= surfelCount)
    {
        return;
    }

    const Surfel surfel = constants.surfels.Load(dispatchThreadID);
    const uint surfelCellIndex = GetCellIndexFromWorldPosition(surfel.worldPosition);
    
    uint tempVal;
    InterlockedAdd(constants.surfelCellCounts, surfelCellIndex, 1);

    // Check neighboring cells
    // * * *
    // * x *
    // * * *

    //uint2 currentCellCoordinate = GetCellCoordinate(surfelCellIndex);
    //
    //[unroll]
    //for (uint i = 0; i < CellNeighbourCount; i++)
    //{
    //    int2 neighbourCell = currentCellCoordinate + m_cellNeighbours[i];
    //    if (all(neighbourCell > 0) && all(neighbourCell < SurfelGridSize))
    //    {
    //        if (IsSurfelIntersectingCell(surfel, neighbourCell))
    //        {
    //            uint neighbourCellIndex = GetCellIndexFromCoordinate(neighbourCell);
    //
    //            uint tempVal;
    //            constants.surfelCellCounts.InterlockedAdd(neighbourCellIndex, 1, tempVal);
    //        }
    //    }
    //}
}