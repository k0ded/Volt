#pragma once

#include <RHIModule/Synchronization/Fence.h>

struct VkSemaphore_T;

namespace Volt::RHI
{
	class VulkanFence : public Fence
	{
	public:
		VulkanFence();
		~VulkanFence() override;

		void WaitUntilSignaled() const override;
		bool IsSignaled() const override;
		void Reset() override;

	protected:
		void* GetHandleImpl() const override;

	private:
		friend class VulkanDeviceQueue;
		friend class VulkanRHISubmissionThread;
		friend class LastSubmissionTracker;
		friend class VulkanCommandBuffer;

		void AssignSemaphore(VkSemaphore_T* semaphore, uint64_t value);
	
		VkSemaphore_T* m_referencedSemaphore = nullptr;
		std::atomic_uint64_t m_referencedValue = 0;
		std::atomic_bool m_hasBeenSubmitted = false;
	};
}
