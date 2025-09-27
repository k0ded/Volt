#pragma once

#include <RHIModule/Synchronization/Fence_New.h>

struct VkSemaphore_T;

namespace Volt::RHI
{
	class VulkanFence_New : public Fence_New
	{
	public:
		VulkanFence_New();
		~VulkanFence_New() override;

		void WaitUntilSignaled() const override;
		bool IsSignaled() const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		friend class VulkanDeviceQueue;
	
		VkSemaphore_T* m_referencedSemaphore = nullptr;
		uint64_t m_referencedValue = 0;
	};
}
