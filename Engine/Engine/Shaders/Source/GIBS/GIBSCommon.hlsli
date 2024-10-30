#pragma once

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

uint2 GetCellCoordinate(uint cellIndex)
{
    uint cellX = cellIndex % SurfelGridSize;
    uint cellY = cellIndex / SurfelGridSize;
    
    return uint2(cellX, cellY);
}

uint GetCellIndexFromCoordinate(uint2 cellCoordinate)
{
    return cellCoordinate.y * SurfelGridSize + cellCoordinate.x;
}

uint GetCellIndexFromWorldPosition(float3 position)
{
    const float halfSize = (SurfelGridSize * SurfelCellSize) / 2.f;

    uint cellX = (uint)(floor((position.x + halfSize) / SurfelCellSize));
    uint cellY = (uint)(floor((position.z + halfSize) / SurfelCellSize));

    return cellY * SurfelGridSize + cellX;
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
