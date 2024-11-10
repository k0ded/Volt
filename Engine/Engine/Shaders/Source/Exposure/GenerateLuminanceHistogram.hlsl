#include "Resources.hlsli"
#include "Atomics.hlsli"

#define THREAD_GROUP_SIZE 16

static const float3 RGBToLum = float3(0.2125f, 0.7154f, 0.0721f);

struct Constants
{
    vt::Tex2D<float3> inputColor;
    vt::RWTypedBuffer<uint> outHistogram;

    uint2 renderTargetSize;
    float minLogLum;
    float inverseLogLumRange;
};

groupshared uint m_groupHistogram[THREAD_GROUP_SIZE * THREAD_GROUP_SIZE];

uint GetBinIndexFromColor(float3 color, float minLogLum, float inverseLogLumRange)
{
    float lum = dot(color, RGBToLum);

    if (lum < 0.0001f)
    {
        return 0.f;
    }

    float logLum = saturate((log2(lum) - minLogLum) * inverseLogLumRange);
    
    return uint(logLum * 254.f + 1.f);
}

[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]
void GenerateLuminanceHistogramCS(uint2 dispatchThreadId : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    const Constants constants = GetConstants<Constants>();

    const uint groupThreadIndex = groupIndex;

    m_groupHistogram[groupThreadIndex] = 0;

    GroupMemoryBarrierWithGroupSync();

    if (all(dispatchThreadId < constants.renderTargetSize))
    {
        float3 color = constants.inputColor.Load(int3(dispatchThreadId, 0));
        uint binIndex = GetBinIndexFromColor(color, constants.minLogLum, constants.inverseLogLumRange);
        
        InterlockedAdd(m_groupHistogram[binIndex], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedAdd(constants.outHistogram, groupThreadIndex, m_groupHistogram[groupThreadIndex]);
}