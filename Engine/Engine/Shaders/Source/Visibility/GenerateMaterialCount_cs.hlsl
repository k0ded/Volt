#include "Structures.hlsli"
#include "Resources.hlsli"
#include "GPUScene.hlsli"
#include "Common.hlsli"
#include "MeshletHelpers.hlsli"

#include "Atomics.hlsli"

GPUScene GPUSceneData;

vt::Tex2D<uint2> VisibilityBuffer;
vt::RWTypedBuffer<uint> MaterialCountsBuffer;

uint2 RenderSize;

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    if (threadId.x >= RenderSize.x || threadId.y >= RenderSize.y)
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
     
    vt::InterlockedAdd(MaterialCountsBuffer, objectData.materialId, 1);    
}