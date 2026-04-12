#include "rhipch.h"

#include "RHIModule/ResourceDeletionQueue.h"

#include <CoreUtilities/Profiling/Profiling.h>

#include <ranges>

namespace Volt::RHI
{
	void ResourceDeletionQueue::EnqueueResourceDeletion(FunctionType&& deletionFunc, IntRef<Fence> waitForFence)
	{
		std::scoped_lock lock{ m_queueMutex };
		VT_PROFILE_LOCK_MARK(m_queueMutex);

		VT_ASSERT(deletionFunc);

		Item& item = m_queue.emplace_back();
		item.func = std::move(deletionFunc);
		item.waitForFence = waitForFence;
	}

	void ResourceDeletionQueue::FlushQueue(bool waitForFences)
	{
		VT_PROFILE_FUNCTION();

		std::scoped_lock lock{ m_queueMutex };
		VT_PROFILE_LOCK_MARK(m_queueMutex);

		for (int32_t i = static_cast<int32_t>(m_queue.size()) - 1; i >= 0; --i)
		{
			Item& item = m_queue[i];

			if (item.waitForFence)
			{
				if (!waitForFences)
				{
					if (!item.waitForFence->IsSignaled())
					{
						continue;
					}
				}
				else
				{
					item.waitForFence->WaitUntilSignaled();
				}
			}

			item.func();

			m_queue.erase_unsorted(m_queue.begin() + i);
		}
	}

	ResourceDeletionQueue::ResourceDeletionQueue(const ResourceDeletionQueue& other)
	{
		m_queue = other.m_queue;
	}

	void ResourceDeletionQueue::FlushAll()
	{
		FlushQueue(true);
	}
}
