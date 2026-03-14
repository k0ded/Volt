#pragma once

#include "JobSystem/Job.h"

#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Containers/Array.h>

namespace Volt
{
	template<typename T, QueueThreadingPolicy ThreadingPolicy>
	class JobPriorityQueue
	{
	public:
		void Allocate(size_t capacity);

		template<typename... Args>
		bool Emplace(ExecutionPriority priority, Args&&... args);
		bool Pop(ExecutionPriority priority, T& outValue);

		size_t Size(ExecutionPriority priority) const;

	private:
		using UnderlyingType = std::underlying_type_t<ExecutionPriority>;

		Array<WorkQueue<T, ThreadingPolicy>, std::to_underlying(ExecutionPriority::Num)> m_queue;
	};

	template<typename T, QueueThreadingPolicy ThreadingPolicy>
	void JobPriorityQueue<T, ThreadingPolicy>::Allocate(size_t capacity)
	{
		for (UnderlyingType i = 0; i < std::to_underlying(ExecutionPriority::Num); ++i)
		{
			m_queue[i].Allocate(capacity);
		}
	}

	template<typename T, QueueThreadingPolicy ThreadingPolicy>
	template<typename... Args>
	bool JobPriorityQueue<T, ThreadingPolicy>::Emplace(ExecutionPriority priority, Args&&... args)
	{
		return m_queue[std::to_underlying(priority)].Emplace(std::forward<Args>(args)...);
	}

	template<typename T, QueueThreadingPolicy ThreadingPolicy>
	bool JobPriorityQueue<T, ThreadingPolicy>::Pop(ExecutionPriority priority, T& outValue)
	{
		return m_queue[std::to_underlying(priority)].Pop(outValue);
	}

	template<typename T, QueueThreadingPolicy ThreadingPolicy>
	size_t JobPriorityQueue<T, ThreadingPolicy>::Size(ExecutionPriority priority) const
	{
		return m_queue[std::to_underlying(priority)].Size();
	}
}
