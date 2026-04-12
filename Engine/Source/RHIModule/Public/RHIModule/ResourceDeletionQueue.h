#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Synchronization/Fence.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Profiling/Profiling.h>

#include <functional>
#include <mutex>

namespace Volt::RHI
{
	class VTRHI_API ResourceDeletionQueue
	{
	public:
		using FunctionType = std::function<void()>;
		using PerFrameQueue = Vector<FunctionType>;

		ResourceDeletionQueue() = default;
		ResourceDeletionQueue(const ResourceDeletionQueue& other);

		void EnqueueResourceDeletion(FunctionType&& deletionFunc, IntRef<Fence> waitForFence);
		void FlushQueue(bool waitForFences = false);
		void FlushAll();

	private:
		struct Item
		{
			FunctionType func;
			IntRef<Fence> waitForFence;
		};

		Vector<Item> m_queue;

		VT_PROFILE_DECLARE_MUTEX(std::mutex, m_queueMutex);
	};
}
