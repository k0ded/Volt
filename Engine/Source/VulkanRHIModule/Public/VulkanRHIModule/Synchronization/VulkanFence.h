#pragma once

#include <RHIModule/Synchronization/Fence.h>

struct VkFence_T;

namespace Volt::RHI
{
	class VulkanFence : public Fence
	{
	public:
		VulkanFence(const FenceCreateInfo& createInfo);
		~VulkanFence() override;

		void Reset() const override;
		FenceStatus GetStatus() const override;
		void WaitUntilSignaled() const override;

	protected:
		friend class VulkanDeviceQueue;
		friend class VulkanSwapchain;

		void* GetHandleImpl() const override;

		void MarkAsExecuted();

	private:
		VkFence_T* m_fence = nullptr;
		mutable std::atomic_bool m_isExecuted = false;
	};
}
