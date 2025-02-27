#pragma once

enum SurfelAllocator
{
    Counter = 0,
    StackPointer = 1,
    ValidSurfelCounter = 2
};

struct Surfel
{
    float3 worldPosition;
    float radius;
    float3 normal;
    float padding;
};

struct SurfelGridCell
{
    uint surfelCount;
    uint cellIndirectOffset;
};

struct CellIndexInfo
{
    int cellIndex : 24;
    int isValid : 8;
};

static const int2 m_cellNeighbours[] =
{
    int2(-1, -1),
    int2(0, -1),
    int2(1, -1),
    int2(-1, 0),
    int2(1, 0),
    int2(-1, 1),
    int2(0, 1),
    int2(1, 1)    
};

static const uint CellNeighbourCount = 8;

static const float SurfelCellSize = 100.f;
static const uint SurfelGridSize = 100;

int2 GetCellCoordinate(uint cellIndex)
{
    uint cellX = cellIndex % SurfelGridSize;
    uint cellY = cellIndex / SurfelGridSize;
    
    return int2(cellX, cellY);
}

CellIndexInfo GetCellIndexFromCellCoordinates(uint2 cellCoordinate)
{
    CellIndexInfo result;
    result.cellIndex = cellCoordinate.y * SurfelGridSize + cellCoordinate.x;
    result.isValid = result.cellIndex >= 0 && result.cellIndex < SurfelGridSize * SurfelGridSize;
    
    return result;
}

int2 GetCellCoordinatesFromWorldPosition(float3 position)
{
    const float halfSize = (SurfelGridSize * SurfelCellSize) / 2.f;
    const float invSurfelCellSize = 1.f / SurfelCellSize;

    uint cellX = (uint)(floor((position.x + halfSize) * invSurfelCellSize));
    uint cellY = (uint)(floor((position.z + halfSize) * invSurfelCellSize));

    cellX = clamp(cellX, 0, SurfelGridSize);
    cellY = clamp(cellY, 0, SurfelGridSize);

    return int2(cellX, cellY);
}

bool IsSurfelIntersectingCell(in Surfel surfel, uint2 cellCoordinate)
{
    const float halfSize = (SurfelGridSize * SurfelCellSize) / 2.f;

    float2 cellMin = float2(cellCoordinate.x * SurfelCellSize - halfSize, cellCoordinate.y * SurfelCellSize - halfSize);
    float2 cellMax = float2((cellCoordinate.x + 1) * SurfelCellSize - halfSize, (cellCoordinate.y + 1) * SurfelCellSize - halfSize);

    float2 surfelPosition = float2(surfel.worldPosition.x, surfel.worldPosition.z);
    
    float closestX = max(cellMin.x, min(surfelPosition.x, cellMax.x));
    float closestY = max(cellMin.y, min(surfelPosition.y, cellMax.y));

    float2 diff = surfelPosition - float2(closestX, closestY);
    
    return dot(diff, diff) <= surfel.radius * surfel.radius;
}

bool IsSurfelIntersectingCell(in Surfel surfel, uint cellIndex)
{
    uint cellX = cellIndex % SurfelGridSize;
    uint cellY = cellIndex / SurfelGridSize;

    return IsSurfelIntersectingCell(surfel, uint2(cellX, cellY));
}
