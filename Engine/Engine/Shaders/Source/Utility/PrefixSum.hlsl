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

struct Constants
{
    vt::TypedBuffer<uint> inputValues;
    vt::RWTypedBuffer<uint> outputValues;
    vt::RWTypedBuffer<State, true> state;
    vt::RWRawByteBuffer counterBuffer;
    uint valueCount;
};

groupshared uint m_wavePrefixSums[TG_WAVE_COUNT];
groupshared uint m_partitionIndex;
groupshared uint m_lookBackFlag;
groupshared uint m_partitionPrefix;

[numthreads(TG_SIZE, 1, 1)]
void main(uint groupThreadId : SV_GroupThreadID)
{
    const Constants constants = GetConstants<Constants>();
    
    // We need to use the raw buffer here.
    globallycoherent RWStructuredBuffer<State> stateBuffer = ResourceDescriptorHeap[constants.state.handle.handle + 1];

    if (groupThreadId == 0)
    {
        constants.counterBuffer.InterlockedAdd(0, 1, m_partitionIndex);
        m_partitionPrefix = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    const uint WAVE_SIZE = WaveGetLaneCount();
    const uint WAVE_INDEX = groupThreadId.x / WAVE_SIZE;
    const uint LANE_INDEX = WaveGetLaneIndex();

    const uint localValueIndex = WAVE_SIZE * WAVE_INDEX + LANE_INDEX;
    const uint valueIndex = TG_SIZE * m_partitionIndex + localValueIndex;

    if (valueIndex >= constants.valueCount)
    {
        return;
    }

    const uint maxLocalIndex = constants.valueCount - TG_SIZE * m_partitionIndex - 1;

    const bool isLastLaneInWave = groupThreadId == (WAVE_INDEX * WAVE_SIZE) + WAVE_SIZE - 1;
    const bool isLastActiveGroupThread = groupThreadId == maxLocalIndex || groupThreadId == TG_SIZE - 1;
    
    uint value = constants.inputValues.Load(valueIndex);
    uint lanePrefixSum = WavePrefixSum(value);

    // Store the per wave prefix sum for the entire thread group.
    if (isLastLaneInWave)
    {
        m_wavePrefixSums[WAVE_INDEX] = lanePrefixSum + value;
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
    if (WAVE_INDEX > 0)
    {
        laneAggregate += m_wavePrefixSums[WAVE_INDEX - 1];
    }

    if (isLastActiveGroupThread)
    {
        stateBuffer[m_partitionIndex].aggregate = laneAggregate + value;
        if (m_partitionIndex == 0)
        {
            stateBuffer[m_partitionIndex].prefix = laneAggregate;
        }
    }

    DeviceMemoryBarrierWithGroupSync();

    if (isLastActiveGroupThread)
    {
        uint state = STATE_AGG_READY;
        if (m_partitionIndex == 0)
        {
            state = STATE_PRE_READY;
        }

        stateBuffer[m_partitionIndex].state = state;
    }
    
    uint exclusivePrefix = 0;

    if (m_partitionIndex > 0)
    {
        int lookBackIndex = m_partitionIndex - 1;

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
                uint otherValue = constants.inputValues.Load(lookBackIndex * TG_SIZE + otherValueIndex);
                
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
            stateBuffer[m_partitionIndex].prefix = exclusivePrefix + m_wavePrefixSums[WAVE_INDEX];
        }

        DeviceMemoryBarrier();
        
        if (isLastActiveGroupThread)                                                                                                                                                                                  
        {
            stateBuffer[m_partitionIndex].state = STATE_PRE_READY;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    constants.outputValues.Store(valueIndex, m_partitionPrefix + laneAggregate);
}