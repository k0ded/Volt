#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Utility/GPUCrashTracker.h>

struct VkDevice_T;

namespace Volt::RHI
{
	class VulkanPhysicalGraphicsDevice;

	class VulkanGraphicsDevice final : public GraphicsDevice
	{
	public:
		VulkanGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo);
		~VulkanGraphicsDevice() override;

		void WaitForIdle();

		RefPtr<DeviceQueue> GetDeviceQueue(QueueType queueType) const override;
		const GraphicsDeviceCapabilities& GetCapabilities() const override;

		RawPtr<VulkanPhysicalGraphicsDevice> GetPhysicalDevice() const;

	protected:
		void* GetHandleImpl() const override;

	private:
		void InitializeCapabilities();

		VkDevice_T* m_device = nullptr;
	
		std::unordered_map<QueueType, RefPtr<DeviceQueue>> m_deviceQueues;

		RawPtr<VulkanPhysicalGraphicsDevice> m_physicalDevice;
		GPUCrashTracker m_deviceCrashTracker{};

		GraphicsDeviceCapabilities m_capabilities;
	};
}
