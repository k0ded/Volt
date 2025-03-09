#include "Resources.hlsli"

#define TG_SIZE 512
#define TG_WAVE_SIZE 32
#define TG_WAVE_COUNT (TG_SIZE / TG_WAVE_SIZE)

#define STATE_NOT_READY 0
#define STATE_AGG_READY 1
#define STATE_PRE_READY 2

struct State
{
    uint aggregate;
    uint prefix;
    uint state;
};

vt::TypedBuffer<uint> InputValues;
vt::RWTypedBuffer<uint> OutputValues;
vt::RWTypedBuffer<State, true> StateBuffer;
vt::RWRawByteBuffer CounterBuffer;
uint ValueCount;

groupshared uint m_wavePrefixSums[TG_WAVE_COUNT];
groupshared uint m_partitionIndex;
groupshared uint m_lookBackFlag;
groupshared uint m_partitionPrefix;

[numthreads(TG_SIZE, 1, 1)]
void main(uint groupThreadId : SV_GroupThreadID)
{
    // We need to use the raw buffer here.
    globallycoherent RWStructuredBuffer<State> stateBuffer = ResourceDescriptorHeap[StateBuffer.handle.handle + 1];

    const uint WaveSize = WaveGetLaneCount();
    const uint WaveIndex = groupThreadId.x / WaveSize;
    const uint LaneIndex = WaveGetLaneIndex();

    if (groupThreadId == 0)
    {
        CounterBuffer.InterlockedAdd(0, 1, m_partitionIndex);
        m_partitionPrefix = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    uint partitionIndex = m_partitionIndex;

    const uint localValueIndex = WaveSize * WaveIndex + LaneIndex;
    const uint valueIndex = TG_SIZE * partitionIndex + localValueIndex;

    if (valueIndex >= ValueCount)
    {
        return;
    }

    const uint maxLocalIndex = ValueCount - TG_SIZE * partitionIndex - 1;

    const bool isLastLaneInWave = groupThreadId == (WaveIndex * WaveSize) + WaveSize - 1 || WaveIndex == (maxLocalIndex / WaveSize);
    const bool isLastActiveGroupThread = groupThreadId == maxLocalIndex || groupThreadId == TG_SIZE - 1;
    
    uint value = InputValues.Load(valueIndex);
    uint lanePrefixSum = WavePrefixSum(value);

    // Store the per wave prefix sum for the entire thread group.
    if (isLastLaneInWave)
    {
        m_wavePrefixSums[WaveIndex] = lanePrefixSum + value;
    }

    GroupMemoryBarrierWithGroupSync();
    
    // Calculate the total wave prefix sum for each wave.
    if (groupThreadId.x == 0)
    {
        [unroll]
        for (uint i = 1; i < TG_WAVE_COUNT; i++)
        {
            m_wavePrefixSums[i] += m_wavePrefixSums[i - 1];
        }
    }

    GroupMemoryBarrierWithGroupSync();

    uint laneAggregate = lanePrefixSum;
    if (WaveIndex > 0)
    {
        laneAggregate += m_wavePrefixSums[WaveIndex - 1];
    }

    if (isLastActiveGroupThread)
    {
        stateBuffer[partitionIndex].aggregate = laneAggregate + value;
        if (partitionIndex == 0)
        {
            stateBuffer[partitionIndex].prefix = laneAggregate;
        }
    }

    DeviceMemoryBarrierWithGroupSync();

    if (isLastActiveGroupThread)
    {
        uint state = STATE_AGG_READY;
        if (partitionIndex == 0)
        {
            state = STATE_PRE_READY;
        }

        stateBuffer[partitionIndex].state = state;
    }
    
    uint exclusivePrefix = 0;

    if (partitionIndex > 0)
    {
        int lookBackIndex = partitionIndex - 1;

        uint otherValueIndex = 0;
        uint otherAggregate = 0;    

        while (true)
        {
            if (isLastActiveGroupThread)
            {
                m_lookBackFlag = stateBuffer[lookBackIndex].state;
            }

            GroupMemoryBarrierWithGroupSync();
            DeviceMemoryBarrier();

            uint lookBackFlag = m_lookBackFlag;

            GroupMemoryBarrierWithGroupSync();
        
            if (lookBackFlag == STATE_PRE_READY)
            {
                if (isLastActiveGroupThread)
                {
                    exclusivePrefix += stateBuffer[lookBackIndex].prefix;
                }

                break;
            }
            else if (lookBackFlag == STATE_AGG_READY)
            {
                if (isLastActiveGroupThread)
                {
                    exclusivePrefix += stateBuffer[lookBackIndex].aggregate;
                }

                lookBackIndex--;
                otherValueIndex = 0;
                continue;
            }

            if (isLastActiveGroupThread)
            {
                uint otherValue = InputValues.Load(lookBackIndex * TG_SIZE + otherValueIndex);
                
                if (otherValueIndex == 0)
                {
                    otherAggregate = otherValue;
                }
                else    
                {
                    otherAggregate += otherValue;
                }

                otherValueIndex++;
                if (otherValueIndex == TG_SIZE)
                {
                    exclusivePrefix += otherAggregate;
                    if (lookBackIndex == 0)
                    {   
                        m_lookBackFlag = STATE_PRE_READY;
                    }
                    else   
                    {
                        lookBackIndex--;
                        otherValueIndex = 0;
                    }
                }
            }

            GroupMemoryBarrierWithGroupSync();
            lookBackFlag = m_lookBackFlag;
            GroupMemoryBarrierWithGroupSync();
            
            if (lookBackFlag == STATE_PRE_READY)
            {
                break;
            }
        }

        if (isLastActiveGroupThread)
        {
            m_partitionPrefix = exclusivePrefix;
            stateBuffer[partitionIndex].prefix = exclusivePrefix + m_wavePrefixSums[WaveIndex];
        }

        DeviceMemoryBarrier();
        
        if (isLastActiveGroupThread)                                                                                                                                                                                  
        {
            stateBuffer[partitionIndex].state = STATE_PRE_READY;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    OutputValues.Store(valueIndex, m_partitionPrefix + laneAggregate);
}