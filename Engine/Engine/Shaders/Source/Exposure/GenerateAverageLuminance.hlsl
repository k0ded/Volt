#include "Resources.hlsli"

#define THREAD_GROUP_SIZE 256

struct Constants
{
    vt::RWTex2D<float> outAverageLuminance;
    vt::RWTypedBuffer<uint> histogramBuffer;

    uint totalPixelCount;
    float logLumRange;
    float minLogLum;
    float blendFactor;
};

groupshared uint m_groupHistogram[THREAD_GROUP_SIZE];

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void GenerateAverageLuminanceCS(uint groupIndex : SV_GroupIndex)
{
    const Constants constants = GetConstants<Constants>();

    uint currentBinCount = constants.histogramBuffer.Load(groupIndex);
    m_groupHistogram[groupIndex] = currentBinCount * groupIndex;

    GroupMemoryBarrierWithGroupSync();
    
    constants.histogramBuffer.Store(groupIndex, 0);

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
        float weightedLogAverage = (m_groupHistogram[0] / max(constants.totalPixelCount - currentBinCount, 1.f)) - 1.f;
        float weightedAvgLum = exp2(((weightedLogAverage / 254.f) * constants.logLumRange) + constants.minLogLum);
        
        float lumLastFrame = constants.outAverageLuminance.Load(int2(0, 0));
        float adaptedLum = lumLastFrame + (weightedAvgLum - lumLastFrame) * constants.blendFactor;
        
        constants.outAverageLuminance.Store(0, adaptedLum);
    }
}