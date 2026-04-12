#pragma once

#include <RHIModule/Synchronization/Fence.h>

struct VkSemaphore_T;

namespace Volt::RHI
{
	class LastSubmissionTracker
	{
	public:
		LastSubmissionTracker();
		virtual ~LastSubmissionTracker() = default;

		void AssignSemaphore(VkSemaphore_T* semaphore, uint64_t value);

	public:
		IntRef<Fence> m_submissionFence;
	};
}
