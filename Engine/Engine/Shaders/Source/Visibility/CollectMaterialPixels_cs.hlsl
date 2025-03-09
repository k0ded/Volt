#include "Common.hlsli"
#include "Resources.hlsli"
#include "GPUScene.hlsli"
#include "MeshletHelpers.hlsli"

#include "Atomics.hlsli"

GPUScene GPUSceneData;

vt::Tex2D<uint2> VisibilityBuffer;
vt::TypedBuffer<uint> MaterialStartBuffer;

vt::RWTypedBuffer<uint> CurrentMaterialCountBuffer;
vt::RWTypedBuffer<uint2> PixelCollectionBuffer;

uint2 RenderSize;

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    if (any(threadId.xy >= RenderSize))
    {
        return;
    }
    
    const uint2 pixelValue = VisibilityBuffer.Load(int3(threadId.xy, 0));
    
    if (pixelValue.x == UINT32_MAX)
    {
        return;
    }
    
    const PrimitiveDrawData objectData = GPUSceneData.primitiveDrawDataBuffer.Load(pixelValue.x);
    if (objectData.materialId == UINT32_MAX)
    {
        return;
    }
    
    uint materialStartIndex = MaterialStartBuffer.Load(objectData.materialId);

    uint currentIndex;
    vt::InterlockedAdd(CurrentMaterialCountBuffer, objectData.materialId, 1, currentIndex);    
    PixelCollectionBuffer.Store(materialStartIndex + currentIndex, threadId.xy);
}