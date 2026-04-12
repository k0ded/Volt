#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Graphics/DeviceQueue.h>

#include <CoreUtilities/Profiling/Profiling.h>

struct VkQueue_T;
struct VkSemaphore_T;
struct VkFence_T;
struct VkCommandBuffer_T;
struct VkSwapchainKHR_T;

namespace Volt::RHI
{
	class VulkanDeviceQueue final : public DeviceQueue
	{
	public:
		VulkanDeviceQueue(const DeviceQueueCreateInfo& createInfo);
		~VulkanDeviceQueue() override;

		void WaitForQueue() override;
		void Execute(const DeviceQueueExecuteInfo& commandBuffer) override;

		void SwapchainExecute(
			VkSemaphore_T* presentSemaphore,
			VkSemaphore_T* renderSemaphore,
			VkFence_T* renderFence,
			VkCommandBuffer_T* commandBuffer);

		void SwapchainPresent(
			VkSwapchainKHR_T* swapchain,
			VkSemaphore_T* renderSemaphore,
			VkFence_T* presentFence,
			uint32_t imageIndex,
			std::mutex* swapchainMutex
		);

		void DestroyQueueSemaphore(class VulkanGraphicsDevice& graphicsDevice);

	protected:
		void* GetHandleImpl() const override;

	private:
		friend class VulkanGraphicsDevice;

		void AquireLock();
		void ReleaseLock();

		void CreateQueueSemaphore(class VulkanGraphicsDevice& graphicsDevice);

		VT_PROFILE_DECLARE_MUTEX(std::mutex, m_executeMutex);

		VkQueue_T* m_queue = nullptr;
		VkSemaphore_T* m_queueSemaphore = nullptr;
		uint64_t m_semaphoreValue = 1;
	};
}
