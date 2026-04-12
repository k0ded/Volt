#include "vkpch.h"

#include "VulkanRHIModule/LastSubmissionTracker.h"
#include "VulkanRHIModule/Synchronization/VulkanFence.h"
#include "VulkanRHIModule/VulkanResourceCast.h"

namespace Volt::RHI
{
	LastSubmissionTracker::LastSubmissionTracker()
	{
		m_submissionFence = Fence::Create();
	}

	void LastSubmissionTracker::AssignSemaphore(VkSemaphore_T* semaphore, uint64_t value)
	{
		VulkanFence* vkFence = ResourceCast(m_submissionFence.GetRaw());
		vkFence->AssignSemaphore(semaphore, value);
	}
}
