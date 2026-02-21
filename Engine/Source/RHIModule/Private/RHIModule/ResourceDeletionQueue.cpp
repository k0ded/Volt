#include "rhipch.h"

#include "RHIModule/ResourceDeletionQueue.h"

#include <CoreUtilities/Profiling/Profiling.h>

#include <ranges>

namespace Volt::RHI
{
	void ResourceDeletionQueue::EnqueueResourceDeletion(uint32_t index, FunctionType&& deletionFunc)
	{
		std::scoped_lock lock{ m_queueMutex };
		VT_PROFILE_LOCK_MARK(m_queueMutex);

		m_queues.at(index).emplace_back(deletionFunc);
	}

	void ResourceDeletionQueue::FlushQueue(uint32_t index)
	{
		VT_PROFILE_FUNCTION();

		std::scoped_lock lock{ m_queueMutex };
		VT_PROFILE_LOCK_MARK(m_queueMutex);

		for (const auto& func : m_queues.at(index))
		{
			if (func)
			{
				func();
			}
		}

		m_queues.at(index).clear();
	}

	void ResourceDeletionQueue::SetSize(uint32_t size)
	{
		m_queues.resize(size);
	}

	ResourceDeletionQueue::ResourceDeletionQueue(const ResourceDeletionQueue& other)
	{
		m_queues = other.m_queues;
	}

	void ResourceDeletionQueue::FlushAll()
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_queues.size()); ++i)
		{
			FlushQueue(i);
		}
	}
}
