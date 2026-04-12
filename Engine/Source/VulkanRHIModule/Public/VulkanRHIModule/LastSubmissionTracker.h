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

		VT_INLINE IntRef<Fence> GetLastSubmissionTrackerFence() const { return m_submissionFence; }

	private:
		friend class LastSubmissionTrackerManager;

		void AssignSemaphore(VkSemaphore_T* semaphore, uint64_t value);
		void MarkAsSubmitted();

		IntRef<Fence> m_submissionFence;
	};
}
