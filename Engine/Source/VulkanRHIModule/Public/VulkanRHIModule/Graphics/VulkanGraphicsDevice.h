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
		VulkanGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer);
		~VulkanGraphicsDevice() override;

		void WaitForIdle();

		RefPtr<DeviceQueue> GetDeviceQueue(QueueType queueType) const override;
		RawPtr<VulkanPhysicalGraphicsDevice> GetPhysicalDevice() const;

		uint64_t GetMaxRequiredStagingBufferSizeForImage(RawPtr<Image> image) const override;
		uint64_t GetRowPitchForWidth(RawPtr<Image> image, uint32_t width) const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		void InitializeCapabilities();

		VkDevice_T* m_device = nullptr;
	
		std::unordered_map<QueueType, RefPtr<DeviceQueue>> m_deviceQueues;

		RawPtr<VulkanPhysicalGraphicsDevice> m_physicalDevice;
		GPUCrashTracker m_deviceCrashTracker{};
	};
}
