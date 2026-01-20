#include "vtcorepch.h"

#include "Volt-Core/Algorithms.h"

#include <Volt-Platforms/Platform.h>

namespace Volt::Algo
{
	void ForEachParalellBlocking(std::function<void(uint32_t threadIdx, uint32_t elementIdx)>&& func, uint32_t iterationCount, uint32_t minInterations)
	{
		if (iterationCount == 0)
		{
			return;
		}

		if (iterationCount < minInterations)
		{
			for (uint32_t i = 0; i < iterationCount; ++i)
			{
				func(0, i);
			}
		}
		else
		{
			const uint32_t threadCount = std::min(iterationCount, PlatformMisc::GetNumberOfLogicalCores());
			const uint32_t perThreadIterationCount = iterationCount / threadCount;

			TaskGraph taskGraph{ ExecutionPriority::Immediate };

			uint32_t iterOffset = 0;
			for (uint32_t i = 0; i < threadCount; i++)
			{
				uint32_t currThreadIterationCount = perThreadIterationCount;
				if (i == threadCount - 1)
				{
					currThreadIterationCount = iterationCount - i * perThreadIterationCount;
				}

				taskGraph.AddTask("ForEach", [currThreadIterationCount, func, iterOffset, i]()
				{
					for (uint32_t iter = 0; iter < currThreadIterationCount; iter++)
					{
						func(i, iter + iterOffset);
					}
				});

				iterOffset += currThreadIterationCount;
			}

			taskGraph.ExecuteAndWait();
		}
	}

	void ForEachParallelAsync(std::function<void(uint32_t, uint32_t)>&& func, uint32_t iterationCount)
	{
		VT_ASSERT_MSG(iterationCount > 0, "Iteration count must be greater than zero!");

		const uint32_t threadCount = std::min(iterationCount, PlatformMisc::GetNumberOfLogicalCores());
		const uint32_t perThreadIterationCount = iterationCount / threadCount;

		Vector<JobRef, InlineAllocator<32>> jobs;

		uint32_t iterOffset = 0;
		for (uint32_t i = 0; i < threadCount; i++)
		{
			uint32_t currThreadIterationCount = perThreadIterationCount;
			if (i == threadCount - 1)
			{
				currThreadIterationCount = iterationCount - i * perThreadIterationCount;
			}

			jobs.emplace_back() = JobSystem::CreateJob("ForEachParallel", ExecutionPriority::Critical, [currThreadIterationCount, func, iterOffset, i]()
			{
				for (uint32_t iter = 0; iter < currThreadIterationCount; iter++)
				{
					func(i, iter + iterOffset);
				}
			});

			iterOffset += currThreadIterationCount;
		}

		JobSystem::RunJobs(jobs);
	}

	uint32_t GetThreadCountFromIterationCount(uint32_t iterationCount)
	{
		VT_ASSERT_MSG(iterationCount > 0, "Iteration count must be greater than zero!");

		const uint32_t threadCount = std::min(iterationCount, PlatformMisc::GetNumberOfLogicalCores());
	
		return threadCount;
	}
}
