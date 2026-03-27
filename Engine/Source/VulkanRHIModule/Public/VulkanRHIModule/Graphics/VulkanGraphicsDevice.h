#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Utility/GPUCrashTracker.h>

#include <CoreUtilities/Containers/Array.h>

struct VkDevice_T;

namespace tracy
{
	class VkCtx;
}

namespace Volt::RHI
{
	class VulkanPhysicalGraphicsDevice;

	class VulkanGraphicsDevice final : public GraphicsDevice
	{
	public:
		VulkanGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer);
		~VulkanGraphicsDevice() override;

		void WaitForIdle();

		IntRef<DeviceQueue> GetDeviceQueue(QueueType queueType) const override;
		RawPtr<VulkanPhysicalGraphicsDevice> GetPhysicalDevice() const;

		uint64_t GetMaxRequiredStagingBufferSizeForImage(RawPtr<Image> image) const override;
		uint64_t GetRowPitchForWidth(RawPtr<Image> image, uint32_t width) const override;
		MemoryRequirement GetImageMemoryRequirement(const ImageDesc& desc) const override;
		MemoryRequirement GetBufferMemoryRequirement(const BufferDesc& desc) const override;

		VT_INLINE bool HasCalibratedTimeDomains() const { return m_hasCalibratedTimeDomains; }
		VT_INLINE tracy::VkCtx* GetProfilingContext() const { return m_profilingContext; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void InitializeCapabilities();
		void InitializeProfilingContext();

		VkDevice_T* m_device = nullptr;
		bool m_hasCalibratedTimeDomains = false;

		Array<IntRef<DeviceQueue>, std::to_underlying(QueueType::Num)> m_deviceQueues;

		RawPtr<VulkanPhysicalGraphicsDevice> m_physicalDevice;
		GPUCrashTracker m_deviceCrashTracker{};

		tracy::VkCtx* m_profilingContext = nullptr;
	};
}
