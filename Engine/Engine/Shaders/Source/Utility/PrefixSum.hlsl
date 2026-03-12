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

Buffer<uint> InputValues;

RWBuffer<uint> RWOutputValues;
RWBuffer<uint> RWCounterBuffer;
globallycoherent RWStructuredBuffer<State> RWStateBuffer;

uint ValueCount;

groupshared uint GroupWavePrefixSums[TG_WAVE_COUNT];
groupshared uint GroupPartitionIndex;
groupshared uint GroupLookBackFlag;
groupshared uint GroupPartitionPrefix;

[numthreads(32, 1, 1)]
void MainCS()
{
    const uint laneIndex = WaveGetLaneIndex();

    uint laneValue = InputValues[laneIndex];
    RWOutputValues[laneIndex] = WavePrefixSum(laneValue);
}

[numthreads(TG_SIZE, 1, 1)]
void MainCS2(uint GroupThreadId : SV_GroupThreadId)
{
    // We need to use the raw buffer here.
    const uint WaveSize = WaveGetLaneCount();
    const uint WaveIndex = GroupThreadId.x / WaveSize;
    const uint LaneIndex = WaveGetLaneIndex();

    if (GroupThreadId == 0)
    {
        InterlockedAdd(RWCounterBuffer[0], 1, GroupPartitionIndex);
        GroupPartitionPrefix = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    uint partitionIndex = GroupPartitionIndex;

    const uint localValueIndex = WaveSize * WaveIndex + LaneIndex;
    const uint valueIndex = TG_SIZE * partitionIndex + localValueIndex;

    if (valueIndex >= ValueCount)
    {
        return;
    }

    const uint maxLocalIndex = ValueCount - TG_SIZE * partitionIndex - 1;

    const bool isLastLaneInWave = GroupThreadId == (WaveIndex * WaveSize) + WaveSize - 1 || WaveIndex == (maxLocalIndex / WaveSize);
    const bool isLastActiveGroupThread = GroupThreadId == maxLocalIndex || GroupThreadId == TG_SIZE - 1;
    
    uint value = InputValues[valueIndex];
    uint lanePrefixSum = WavePrefixSum(value);

    // Store the per wave prefix sum for the entire thread group.
    if (isLastLaneInWave)
    {
        GroupWavePrefixSums[WaveIndex] = lanePrefixSum + value;
    }

    GroupMemoryBarrierWithGroupSync();
    
    // Calculate the total wave prefix sum for each wave.
    if (GroupThreadId.x == 0)
    {
        [unroll]
        for (uint i = 1; i < TG_WAVE_COUNT; i++)
        {
            GroupWavePrefixSums[i] += GroupWavePrefixSums[i - 1];
        }
    }

    GroupMemoryBarrierWithGroupSync();

    uint laneAggregate = lanePrefixSum;
    if (WaveIndex > 0)
    {
        laneAggregate += GroupWavePrefixSums[WaveIndex - 1];
    }

    if (isLastActiveGroupThread)
    {
        RWStateBuffer[partitionIndex].aggregate = laneAggregate + value;
        if (partitionIndex == 0)
        {
            RWStateBuffer[partitionIndex].prefix = laneAggregate;
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

        RWStateBuffer[partitionIndex].state = state;
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
                GroupLookBackFlag = RWStateBuffer[lookBackIndex].state;
            }

            GroupMemoryBarrierWithGroupSync();
            DeviceMemoryBarrier();

            uint lookBackFlag = GroupLookBackFlag;

            GroupMemoryBarrierWithGroupSync();
        
            if (lookBackFlag == STATE_PRE_READY)
            {
                if (isLastActiveGroupThread)
                {
                    exclusivePrefix += RWStateBuffer[lookBackIndex].prefix;
                }

                break;
            }
            else if (lookBackFlag == STATE_AGG_READY)
            {
                if (isLastActiveGroupThread)
                {
                    exclusivePrefix += RWStateBuffer[lookBackIndex].aggregate;
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
                        GroupLookBackFlag = STATE_PRE_READY;
                    }
                    else   
                    {
                        lookBackIndex--;
                        otherValueIndex = 0;
                    }
                }
            }

            GroupMemoryBarrierWithGroupSync();
            lookBackFlag = GroupLookBackFlag;
            GroupMemoryBarrierWithGroupSync();
            
            if (lookBackFlag == STATE_PRE_READY)
            {
                break;
            }
        }

        if (isLastActiveGroupThread)
        {
            GroupPartitionPrefix = exclusivePrefix;
            RWStateBuffer[partitionIndex].prefix = exclusivePrefix + GroupWavePrefixSums[WaveIndex];
        }

        DeviceMemoryBarrier();
        
        if (isLastActiveGroupThread)                                                                                                                                                                                  
        {
            RWStateBuffer[partitionIndex].state = STATE_PRE_READY;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    RWOutputValues[valueIndex] = GroupPartitionPrefix + laneAggregate;
}