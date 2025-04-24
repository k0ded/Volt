#include "Resources.hlsli"
#include "GPUScene.hlsli"
#include "Structures.hlsli"

#include "MeshShaderConfig.hlsli"
#include "Utility.hlsli"

#include "Atomics.hlsli"

namespace CullingType
{
    static const uint Perspective = 0;
    static const uint Orthographic = 1;
}

vt::RWTypedBuffer<uint> CountBuffer;
vt::RWTypedBuffer<MeshTaskCommand> TaskCommands;

GPUScene GPUSceneData;

float4x4 ViewMatrix;
float4 CullingFrustum;
float NearPlane;
float FarPlane;

uint CullingTypeInt;

[numthreads(64, 1, 1)]
void MainCS(uint dispatchThreadId : SV_DispatchThreadID)
{
    const uint validPrimitiveDrawDataCount = GPUSceneData.validPrimitiveDrawDatasBuffer.Load(0);

    if (dispatchThreadId >= validPrimitiveDrawDataCount)
    {
        return;
    }

    // We must offset by one as the first element is the counter.
    const uint primitiveDrawDataIndex = GPUSceneData.validPrimitiveDrawDatasBuffer.Load(dispatchThreadId + 1);
    const PrimitiveDrawData drawData = GPUSceneData.primitiveDrawDataBuffer.Load(primitiveDrawDataIndex);
    const GPUMesh mesh = GPUSceneData.meshesBuffer.Load(drawData.meshId);

    const float3 center = mul(ViewMatrix, float4(drawData.transform.GetWorldPosition(mesh.boundingSphere.center), 1.f)).xyz;
    const float radius = mesh.boundingSphere.radius * max(drawData.transform.scale.x, max(drawData.transform.scale.y, drawData.transform.scale.z));

    bool visible = true;

    if (CullingTypeInt == CullingType::Perspective)
    {
        visible = visible && center.z * CullingFrustum.y - abs(center.x) * CullingFrustum.x > -radius;
        visible = visible && center.z * CullingFrustum.w - abs(center.y) * CullingFrustum.z > -radius;
        visible = visible && center.z + radius > NearPlane && center.z - radius < FarPlane;
    }
    else if (CullingTypeInt == CullingType::Orthographic)
    {
        //float closestX = clamp(center.x, constants.cullingFrustum.x, constants.cullingFrustum.y);
        //float closestY = clamp(center.y, constants.cullingFrustum.z, constants.cullingFrustum.w);
        //
        //float distanceX = center.x - closestX;
        //float distanceY = center.y - closestY;
        //
        //float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
        //visible = visible && distanceSquared < (radius * radius);
    } 
    
    if (visible)
    {
        uint taskGroups = DivideRoundUp(mesh.meshletCount, NUM_AS_THREADS);
        
        uint drawOffset;
        vt::InterlockedAdd(CountBuffer, 0, taskGroups, drawOffset);

        // Skip draw calls if AS group count limit is reached. This equals ~4M visible draws or ~32B visible triangles.
        if (drawOffset + taskGroups <= NUM_MAX_AS_GROUP_COUNT)
        {
            for (uint i = 0; i < taskGroups; i++)
            {
                MeshTaskCommand command;
                command.drawId = primitiveDrawDataIndex;
                command.taskCount = min(mesh.meshletCount - i * NUM_AS_THREADS, NUM_AS_THREADS);
                command.meshletOffset = i * NUM_AS_THREADS;
                
                TaskCommands.Store(drawOffset + i, command);
            }
        }
    }
}