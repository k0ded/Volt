#pragma once

#include "Utility/BoundingVolumes.hlsli"
#include "Utility/Transform.hlsli"
#include "Lights/Lights.hlsli"

#define MAX_LOD_COUNT 8

namespace PrimitiveFlags
{
    static const uint None = 0;
    static const uint Valid = 1 << 0;
    static const uint Invalid = 1 << 1;
}

struct VertexTriangleCount
{
    uint vertexCount : 16;
    uint triangleCount : 16;
};

struct MeshletCone
{
    uint x : 8;
    uint y : 8;
    uint z : 8;
    uint cutoff : 8;
};

struct Meshlet
{
    VertexTriangleCount vertTriCount;
    uint meshId;
    uint dataOffset;
    MeshletCone cone;

    float3 boundingSphereCenter;
    float boundingSphereRadius;

    uint GetVertexCount()
    {
        return vertTriCount.vertexCount;
    }

    uint GetTriangleCount()
    {
        return vertTriCount.triangleCount;
    }

    uint GetVertexOffset()
    {
        return dataOffset;
    }

    uint GetIndexOffset()
    {
        return dataOffset + GetVertexCount();
    }

    float3 GetConeAxis()
    {
        return float3(cone.x / 127.f, cone.y / 127.f, cone.z / 127.f);
    }

    float GetConeCutoff()
    {
        return float(cone.cutoff / 127.f);
    }
};

struct PrimitiveDrawData
{
    Transform transform;

    uint meshId;
    uint materialId;
    uint meshletStartOffset;
    uint entityId;
    
    uint isAnimated;
    uint boneOffset;
    uint flags;
    uint2 padding;
};

struct GPUMesh
{
    BoundingSphere boundingSphere;

	uint32_t vertexStartOffset;
	uint32_t indexStartOffset;
	
	// Ray Tracing
	uint32_t RT_vertexPositionsBuffer;
	uint32_t RT_vertexMaterialBuffer;
	uint32_t RT_vertexAnimationInfoBuffer;
	uint32_t RT_IndexBuffer;
};

StructuredBuffer<PrimitiveDrawData> PrimitiveDrawDataBuffer;
StructuredBuffer<PrimitiveDrawData> PrevPrimitiveDrawDataBuffer;
StructuredBuffer<GPUMesh> GPUMeshes;
StructuredBuffer<LightDrawData> SceneLights;
StructuredBuffer<float4x4> AnimatedBones;
