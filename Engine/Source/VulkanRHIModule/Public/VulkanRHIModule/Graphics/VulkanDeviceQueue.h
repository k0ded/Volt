#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Graphics/DeviceQueue.h>

struct VkQueue_T;
struct VkSemaphore_T;

namespace Volt::RHI
{
	class VulkanDeviceQueue final : public DeviceQueue
	{
	public:
		VulkanDeviceQueue(const DeviceQueueCreateInfo& createInfo);
		~VulkanDeviceQueue() override;

		void WaitForQueue() override;
		void Execute(const DeviceQueueExecuteInfo& commandBuffer) override;

		void AquireLock();
		void ReleaseLock();

		void DestroyQueueSemaphore(class VulkanGraphicsDevice& graphicsDevice);

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateQueueSemaphore(class VulkanGraphicsDevice& graphicsDevice);

		std::mutex m_executeMutex{};

		VkQueue_T* m_queue = nullptr;
		VkSemaphore_T* m_queueSemaphore = nullptr;
		uint64_t m_semaphoreValue = 1;
	};
}
