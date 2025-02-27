#include "Resources.hlsli"

struct Constants
{
    vt::RWTex3D<float4> rwSpatialFilteredScattering;
    vt::Tex3D<float4> lightScattering;
    vt::TextureSampler pointSampler;

    int3 froxelVolumeDimensions;
};

float GaussianFilter(float radius, float sigma)
{
    const float v = radius / sigma;
    return exp(-(v * v));
}

static const float SigmaFilter = 4.f;
static const int Radius = 2;

[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();
    
    const float3 rcpFroxelDimensions = 1.f / float3(constants.froxelVolumeDimensions);
    float4 scatteringExinction = constants.lightScattering.SampleLevel(constants.pointSampler, float3(dispatchThreadID) * rcpFroxelDimensions, 0.f);

    float accumulatedWeight = 0.f;
    float4 accumulatedScatteringExtinction = 0.f;

    for (int i = -Radius; i <= Radius; ++i)
    {
        for (int j = -Radius; j <= Radius; ++j)
        {
            int3 coord = dispatchThreadID + int3(i, j, 0);

            if (all(coord > 0) && all(coord < constants.froxelVolumeDimensions))
            {
                const float weight = GaussianFilter(length(int2(i, j)), SigmaFilter);
                const float4 sample = constants.lightScattering.SampleLevel(constants.pointSampler, float3(coord) * rcpFroxelDimensions, 0.f);
                accumulatedScatteringExtinction += sample * weight;
                accumulatedWeight += weight;
            }
        }
    }

    scatteringExinction = accumulatedScatteringExtinction / max(accumulatedWeight, 0.00001f);
    constants.rwSpatialFilteredScattering.Store(dispatchThreadID, scatteringExinction);
}