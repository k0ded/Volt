#include "Resources.hlsli"
#include "Structures.hlsli"
#include "RayTracing.hlsli"
#include "GPUScene.hlsli"
#include "VisibilityBuffer.hlsli"
#include "MathConstants.hlsli"

#include "MonteCarlo.hlsli"

struct Constants
{
    vt::UniformBuffer<ViewData> viewData;
    vt::RWTex2D<float3> rwProbeIrradianceAtlas;

    GPUScene gpuScene;

    float probeSpacing;
    uint probeGridSize;
    uint probeResolution;
};

[numthreads(64, 1, 1)]
void main(uint groupThreadId : SV_GroupThreadID)
{
    const Constants constants = GetConstants<Constants>();
 
    uint2 probeCoords = uint2(groupThreadId % constants.probeResolution, groupThreadId / constants.probeResolution);
    float2 probeUv = float2(probeCoords) / float(constants.probeResolution);

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
        constants.rwProbeIrradianceAtlas.Store(probeCoords, 1.f);
    }
}