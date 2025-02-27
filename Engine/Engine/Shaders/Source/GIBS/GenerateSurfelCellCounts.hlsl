#include "Resources.hlsli"
#include "Atomics.hlsli"

#include "GIBSCommon.hlsli"

struct Constants
{
    vt::RWTypedBuffer<uint> surfelCellCounts;

    vt::TypedBuffer<Surfel> surfels;
    vt::TypedBuffer<uint> surfelAllocator;
    vt::TypedBuffer<uint> allocatedSurfels;
};

[numthreads(64, 1, 1)]
void GenerateSurfelCellCounts(uint dispatchThreadID : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();
    uint surfelCount = constants.surfelAllocator.Load(SurfelAllocator::Counter);

    if (dispatchThreadID < surfelCount)
    {
        const uint surfelIndex = constants.allocatedSurfels.Load(dispatchThreadID);
        const Surfel surfel = constants.surfels.Load(surfelIndex);
        
        const int2 cellCoordinates = GetCellCoordinatesFromWorldPosition(surfel.worldPosition);
        const CellIndexInfo cellIndexInfo = GetCellIndexFromCellCoordinates(cellCoordinates);

        if (cellIndexInfo.isValid)
        {
            InterlockedAdd(constants.surfelCellCounts, cellIndexInfo.cellIndex, 1);

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
    }
}