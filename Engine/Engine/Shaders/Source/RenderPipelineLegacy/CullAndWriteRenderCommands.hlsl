#include "RenderScene/GPUScene.hlsli"
#include "IndirectCommands.hlsli"

RWBuffer<uint> RWDrawCommands;

Buffer<uint> PrebuiltDrawCommands;
Buffer<uint> DrawCommandsToCopyIndices;

uint NumCommandsToCopy;

[numthreads(64, 1, 1)]
void CopyIndirectDrawCommandsCS(uint DispatchThreadID : SV_DispatchThreadID)
{
    if (DispatchThreadID < NumCommandsToCopy)
    {
        const uint toCopyIndex = DrawCommandsToCopyIndices[DispatchThreadID];
        const uint copySrcOffset = toCopyIndex * DrawIndexedIndirectCommand::SizeInUInts;
        const uint copyDstOffset = DispatchThreadID * DrawIndexedIndirectCommand::SizeInUInts;

        for (uint i = 0; i < DrawIndexedIndirectCommand::SizeInUInts; ++i)
        {
            RWDrawCommands[copyDstOffset + i] = PrebuiltDrawCommands[copySrcOffset + i];
        }
    }
}

enum ECullingType : uint
{
    Perspective = 0,
    Orthographic = 1 
};

RWBuffer<uint> RWPerMeshDrawCommandCount;
RWBuffer<uint> RWPrimitivesToDrawCounter;
RWBuffer<uint> RWPrimitivesToDraw;
Buffer<uint> ValidPrimitiveDrawDataIndices;
Buffer<int> PrimitiveIndexToDrawCommandIndex;

float4x4 ViewMatrix;
float4 CullingFrustum;
float NearPlane;
float FarPlane;
uint CullingType;

[numthreads(256, 1, 1)]
void CullRenderPrimitivesCS(uint DispatchThreadID : SV_DispatchThreadID)
{
    const uint numValidPrimitiveDrawDatas = ValidPrimitiveDrawDataIndices[0];
    if (DispatchThreadID < numValidPrimitiveDrawDatas)
    {
        const uint primitiveDrawDataIndex = ValidPrimitiveDrawDataIndices[DispatchThreadID + 1];
        const int drawCommandIndex = PrimitiveIndexToDrawCommandIndex[primitiveDrawDataIndex];
       
        if (drawCommandIndex != -1)
        {
            const PrimitiveDrawData primtiveDrawData = PrimitiveDrawDataBuffer[primitiveDrawDataIndex];
            const GPUMesh gpuMesh = GPUMeshes[primtiveDrawData.meshId];
            
            const float3 center = mul(ViewMatrix, float4(primtiveDrawData.transform.GetWorldPosition(gpuMesh.boundingSphere.center), 1.f)).xyz;
            const float radius = gpuMesh.boundingSphere.radius * max(primtiveDrawData.transform.scale.x, max(primtiveDrawData.transform.scale.y, primtiveDrawData.transform.scale.z));
    
            bool visible = true;

            if (CullingType == ECullingType::Perspective)
            {
                visible = visible && center.z * CullingFrustum.y - abs(center.x) * CullingFrustum.x > -radius;
                visible = visible && center.z * CullingFrustum.w - abs(center.y) * CullingFrustum.z > -radius;
                visible = visible && center.z + radius > NearPlane && center.z - radius < FarPlane;
            }

            if (visible)
            {
                InterlockedAdd(RWPerMeshDrawCommandCount[drawCommandIndex], 1);
                
                uint offset;
                InterlockedAdd(RWPrimitivesToDrawCounter[0], 1, offset);
                RWPrimitivesToDraw[offset] = primitiveDrawDataIndex;
            }
        }
    }
}

RWBuffer<uint> RWPrimitiveDrawDataIndirection;

Buffer<uint> PrimitiveStartOffset;
Buffer<uint> PrimitivesToDraw;
Buffer<uint> PrimitivesToDrawCounter;

[numthreads(256, 1, 1)]
void CompactAndWriteRenderCommandsCS(uint DispatchThreadID : SV_DispatchThreadID)
{
    if (DispatchThreadID < PrimitivesToDrawCounter[0])
    {
        const uint primitiveIndex = PrimitivesToDraw[DispatchThreadID];
        const uint drawCommandIndex = PrimitiveIndexToDrawCommandIndex[primitiveIndex];

        const uint primitiveStartOffset = PrimitiveStartOffset[drawCommandIndex];
        const uint commandOffset = DrawIndexedIndirectCommand::SizeInUInts * drawCommandIndex;
        
        // Find the local index of this primitive, 1 is where the index count is located.
        uint localOffset;
        InterlockedAdd(RWDrawCommands[commandOffset + 1], 1, localOffset);

        RWPrimitiveDrawDataIndirection[primitiveStartOffset + localOffset] = primitiveIndex;
        
        // Write the firstInstance parameter of the command.
        if (localOffset == 0)
        {
            RWDrawCommands[commandOffset + 4] = primitiveStartOffset;
        }
    }
}