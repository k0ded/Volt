#include "Resources.hlsli"

#include "GIBSCommon.hlsli"
#include "Atomics.hlsli"

struct Constants
{
    vt::RWTypedBuffer<uint> surfelCellIndirections;
    vt::RWTypedBuffer<uint> surfelCellCounts;

    vt::TypedBuffer<uint> surfelCellPrefixSums;

    vt::TypedBuffer<Surfel> surfels;
    vt::TypedBuffer<uint> surfelAllocator;
    vt::TypedBuffer<uint> allocatedSurfels;
};

[numthreads(64, 1, 1)]
void GenerateSurfelGridIndirection(uint dispatchThreadID : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();
    uint surfelCount = constants.surfelAllocator.Load(SurfelAllocator::Counter);

    if (dispatchThreadID < surfelCount)
    {
        const uint surfelIndex = constants.allocatedSurfels.Load(dispatchThreadID);
        const Surfel surfel = constants.surfels.Load(surfelIndex);
        const uint2 cellCoordinates = GetCellCoordinatesFromWorldPosition(surfel.worldPosition);
        const CellIndexInfo cellIndexInfo = GetCellIndexFromCellCoordinates(cellCoordinates);

        if (cellIndexInfo.isValid)
        {
            const uint surfelCellStartOffset = constants.surfelCellPrefixSums.Load(cellIndexInfo.cellIndex);

            uint index;
            InterlockedAdd(constants.surfelCellCounts, cellIndexInfo.cellIndex, 1, index);
            constants.surfelCellIndirections.Store(surfelCellStartOffset + index, surfelIndex);

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
            //            uint neighbourCellStartOffset = constants.surfelCellPrefixSums.Load(surfelCellIndex);                
            //
            //            uint neighbourIndex;
            //            constants.surfelCellCounts.InterlockedAdd(neighbourCellIndex, 1, neighbourIndex);
            //            constants.surfelCellIndirections.Store(neighbourCellStartOffset + neighbourIndex, dispatchThreadID);
            //        }
            //    }
            //}
        }
    }
}