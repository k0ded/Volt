#pragma once

#include "GPUScene.hlsli"

#define BINDLESS_ENABLED_ 0

#if BINDLESS_ENABLED_
struct GPUMaterial
{
    vt::Tex2D<float4> textures[16];
    vt::TextureSampler samplers[16];
    
    uint textureCount;
    uint materialFlags;
    uint2 padding;
};

struct GPUMesh
{
    vt::TypedBuffer<float3> vertexPositionsBuffer;
    vt::TypedBuffer<VertexMaterialData> vertexMaterialBuffer;
    vt::TypedBuffer<VertexAnimationData> vertexAnimationInfoBuffer;
    vt::TypedBuffer<uint16_t> vertexBoneInfluencesBuffer;
    vt::TypedBuffer<uint> indexBuffer;

    vt::TypedBuffer<float> vertexBoneWeightsBuffer; // Should be packed
    vt::TypedBuffer<uint> meshletDataBuffer;
    vt::TypedBuffer<Meshlet> meshletsBuffer;
    
    BoundingSphere boundingSphere;

    uint vertexStartOffset;
    uint meshletCount;
    uint meshletStartOffset;
    uint meshletIndexStartOffset;
};

struct GPUSDFBrick
{
    float3 min;
    float3 max;
    float3 localCoords;
};

struct GPUMeshSDF
{
    vt::Tex3D<float> sdfTexture;
    float3 size;

    float3 min;
    float3 max;
    
    vt::TypedBuffer<GPUSDFBrick> bricksBuffer;
    uint brickCount;
};

struct GPUScene
{
    vt::TypedBuffer<GPUMesh> meshesBuffer;
    vt::TypedBuffer<GPUMeshSDF> sdfMeshesBuffer;
    vt::TypedBuffer<GPUMaterial> materialsBuffer;
    vt::TypedBuffer<PrimitiveDrawData> primitiveDrawDataBuffer;
    vt::TypedBuffer<PrimitiveDrawData> prevPrimitiveDrawDataBuffer;
    vt::TypedBuffer<float4x4> bonesBuffer;
    vt::TypedBuffer<LightDrawData> lightsBuffer;

    vt::TypedBuffer<uint> validPrimitiveDrawDatasBuffer;
};
#endif