#include "Resources.hlsli"

vt::RWTex3D<float4> RWSpatialFilteredScattering;
vt::Tex3D<float4> LightScattering;
vt::TextureSampler PointSampler;

int3 FroxelVolumeDimensions;

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
    const float3 rcpFroxelDimensions = 1.f / float3(FroxelVolumeDimensions);
    float4 scatteringExinction = LightScattering.SampleLevel(PointSampler, float3(dispatchThreadID) * rcpFroxelDimensions, 0.f);

    float accumulatedWeight = 0.f;
    float4 accumulatedScatteringExtinction = 0.f;

    for (int i = -Radius; i <= Radius; ++i)
    {
        for (int j = -Radius; j <= Radius; ++j)
        {
            int3 coord = dispatchThreadID + int3(i, j, 0);

            if (all(coord > 0) && all(coord < FroxelVolumeDimensions))
            {
                const float weight = GaussianFilter(length(int2(i, j)), SigmaFilter);
                const float4 sample = LightScattering.SampleLevel(PointSampler, float3(coord) * rcpFroxelDimensions, 0.f);
                accumulatedScatteringExtinction += sample * weight;
                accumulatedWeight += weight;
            }
        }
    }

    scatteringExinction = accumulatedScatteringExtinction / max(accumulatedWeight, 0.00001f);
    RWSpatialFilteredScattering.Store(dispatchThreadID, scatteringExinction);
}