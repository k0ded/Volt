#include "vkpch.h"

#include "VulkanRHIModule/LastSubmissionTrackerManager.h"

namespace Volt::RHI
{
	void LastSubmissionTrackerManager::AssignSemaphore(VkSemaphore_T* semaphore, uint64_t value)
	{
		VT_PROFILE_FUNCTION();

		for (const TrackerContainer& container : m_registeredTrackers)
		{
			container.submissionTracker->AssignSemaphore(semaphore, value);
		}

		for (const TrackerContainer& container : m_registeredTrackersArena)
		{
			container.submissionTracker->AssignSemaphore(semaphore, value);
		}
	}

	void LastSubmissionTrackerManager::Reset()
	{
		m_registeredTrackers.clear();
		m_registeredTrackersArena.clear();
	}

	void LastSubmissionTrackerManager::MarkAsSubmitted()
	{
		VT_PROFILE_FUNCTION();

		for (const TrackerContainer& container : m_registeredTrackers)
		{
			container.submissionTracker->MarkAsSubmitted();
		}

		for (const TrackerContainer& container : m_registeredTrackersArena)
		{
			container.submissionTracker->MarkAsSubmitted();
		}
	}

	LastSubmissionTrackerManager::ExtractedTrackers LastSubmissionTrackerManager::ExtractTrackers()
	{
		return { std::move(m_registeredTrackers), std::move(m_registeredTrackersArena) };
	}
}
