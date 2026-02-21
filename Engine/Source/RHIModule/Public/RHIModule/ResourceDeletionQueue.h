#pragma once

#include "RHIModule/Core/Core.h"

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

		void SetSize(uint32_t size);
		void EnqueueResourceDeletion(uint32_t index, FunctionType&& deletionFunc);
		void FlushQueue(uint32_t index);
		void FlushAll();

	private:
		Vector<PerFrameQueue> m_queues;
		VT_PROFILE_DECLARE_MUTEX(std::mutex, m_queueMutex);
	};
}
