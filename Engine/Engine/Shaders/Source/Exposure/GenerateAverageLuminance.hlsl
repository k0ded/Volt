#include "Resources.hlsli"

#define THREAD_GROUP_SIZE 256

vt::RWTex2D<float> RWAverageLuminance;
vt::RWTypedBuffer<uint> RWHistogramBuffer;

uint TotalPixelCount;
float LogLumRange;
float MinLogLum;
float BlendFactor;

groupshared uint m_groupHistogram[THREAD_GROUP_SIZE];

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void GenerateAverageLuminanceCS(uint groupIndex : SV_GroupIndex)
{
    uint currentBinCount = RWHistogramBuffer.Load(groupIndex);
    m_groupHistogram[groupIndex] = currentBinCount * groupIndex;

    GroupMemoryBarrierWithGroupSync();
    
    RWHistogramBuffer.Store(groupIndex, 0);

    [unroll]
    for (uint cutoff = (THREAD_GROUP_SIZE >> 1); cutoff > 0; cutoff >>= 1)
    {  
        if (groupIndex < cutoff)
        {  
            m_groupHistogram[groupIndex] += m_groupHistogram[groupIndex + cutoff];
        }

        GroupMemoryBarrierWithGroupSync();
    }

    if (groupIndex == 0)
    {
        float weightedLogAverage = (m_groupHistogram[0] / max(TotalPixelCount - currentBinCount, 1.f)) - 1.f;
        float weightedAvgLum = exp2(((weightedLogAverage / 254.f) * LogLumRange) + MinLogLum);
        
        float lumLastFrame = RWAverageLuminance.Load(int2(0, 0));
        float adaptedLum = lumLastFrame + (weightedAvgLum - lumLastFrame) * BlendFactor;
        
        RWAverageLuminance.Store(0, adaptedLum);
    }
}