#include "Resources.hlsli"
#include "Atomics.hlsli"

#define THREAD_GROUP_SIZE 16

static const float3 RGBToLum = float3(0.2125f, 0.7154f, 0.0721f);

vt::Tex2D<float3> InputColor;
vt::RWTypedBuffer<uint> RWHistogram;

uint2 RenderTargetSize;
float MinLogLum;
float InverseLogLumRange;

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
    const uint groupThreadIndex = groupIndex;

    m_groupHistogram[groupThreadIndex] = 0;

    GroupMemoryBarrierWithGroupSync();

    if (all(dispatchThreadId < RenderTargetSize))
    {
        float3 color = InputColor.Load(int3(dispatchThreadId, 0));
        uint binIndex = GetBinIndexFromColor(color, MinLogLum, InverseLogLumRange);
        
        InterlockedAdd(m_groupHistogram[binIndex], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedAdd(RWHistogram, groupThreadIndex, m_groupHistogram[groupThreadIndex]);
}