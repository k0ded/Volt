#pragma once

#include "CoreModule/Config.h"

#include <JobSystem/TaskGraph.h>

namespace Volt::Algo
{
	//will dispatch the iterations to the job system and wait until they finish
	extern VTC_API void ForEachParalellBlocking(std::function<void(uint32_t threadIdx, uint32_t elementIdx)>&& func, uint32_t iterationCount, uint32_t minIterations = 0);
	//will dispatch the iterations to the job system but will not wait for them to finish
	extern VTC_API void ForEachParallelAsync(std::function<void(uint32_t, uint32_t)>&& func, uint32_t iterationCount, ExecutionPriority priority = ExecutionPriority::Critical);
	extern VTC_API VT_NODISCARD uint32_t GetThreadCountFromIterationCount(uint32_t iterationCount);

	template<typename T>
	extern VT_NODISCARD Vector<uint32_t> ElementCountPrefixSum(const Vector<Vector<T>>& elements)
	{
		Vector<uint32_t> prefixSums(elements.size());

		prefixSums.resize(elements.size());
		prefixSums[0] = 0;

		for (size_t i = 1; i < elements.size(); i++)
		{
			size_t sum = 0;

			for (size_t j = 0; j < i; j++)
			{
				sum += elements.at(j).size();
			}

			prefixSums[i] = static_cast<uint32_t>(sum);
		}

		return prefixSums;
	}
}
