#include "Resources.hlsli"
#include "Structures.hlsli"
#include "RayTracing.hlsli"
#include "GPUScene.hlsli"
#include "VisibilityBuffer.hlsli"
#include "MathConstants.hlsli"

#include "MonteCarlo.hlsli"

 vt::UniformBuffer<ViewData> View;
 vt::RWTex2D<float3> RWProbeIrradianceAtlas;

 GPUScene GPUSceneData;

 float ProbeSpacing;
 uint ProbeGridSize;
 uint ProbeResolution;

[numthreads(64, 1, 1)]
void main(uint groupThreadId : SV_GroupThreadID)
{
    uint2 probeCoords = uint2(groupThreadId % ProbeResolution, groupThreadId / ProbeResolution);
    float2 probeUv = float2(probeCoords) / float(ProbeResolution);

    float3 traceDirection = EquiAreaSphericalMapping(probeUv);

    RayQuery<RAY_FLAG_FORCE_OPAQUE> query; 
    
    RayDesc rayDesc;
    rayDesc.Origin = float3(0.f, 10.f, 0.f);
    rayDesc.Direction = traceDirection;
    rayDesc.TMin = 0.f;
    rayDesc.TMax = 100000.f;
    
    query.TraceRayInline(g_accelerationStructure, RAY_FLAG_NONE, 0xFF, rayDesc);
    query.Proceed();
    
    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        RWProbeIrradianceAtlas.Store(probeCoords, 1.f);
    }
}