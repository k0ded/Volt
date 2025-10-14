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
	
		VkSemaphore_T* m_referencedSemaphore = nullptr;
		uint64_t m_referencedValue = 0;
	};
}
