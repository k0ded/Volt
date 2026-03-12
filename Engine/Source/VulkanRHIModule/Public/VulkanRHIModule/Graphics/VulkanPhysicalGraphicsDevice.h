#pragma once

#include "VulkanRHIModule/Core.h"
#include "VulkanRHIModule/Graphics/PhysicalDeviceProperties.h"

#include <RHIModule/Graphics/PhysicalGraphicsDevice.h>

struct VkPhysicalDevice_T;

namespace Volt::RHI
{
	struct PhysicalDeviceQueueFamilyIndices
	{
		int32_t graphicsFamilyQueueIndex = -1;
		int32_t computeFamilyQueueIndex = -1;
		int32_t transferFamilyQueueIndex = -1;
	};

	struct ExtensionProperties
	{
		inline static constexpr uint32_t MAX_EXTENSION_NAME_SIZE = 256;

		char extensionName[MAX_EXTENSION_NAME_SIZE];
		uint32_t specVersion;
	};

	class VulkanPhysicalGraphicsDevice final : public PhysicalGraphicsDevice
	{
	public:
		VulkanPhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer);
		~VulkanPhysicalGraphicsDevice() override;

		VT_NODISCARD VT_INLINE std::string_view GetDeviceName() const override { return g_physicalDeviceProperties.deviceName; }
		VT_NODISCARD VT_INLINE const DeviceVendor GetDeviceVendor() const override { return g_physicalDeviceProperties.vendor; }
		VT_NODISCARD VT_INLINE const PhysicalDeviceProperties& GetProperties() const { return g_physicalDeviceProperties; }

		inline const PhysicalDeviceQueueFamilyIndices& GetQueueFamilies() const { return m_queueFamilyIndices; }
		inline const PhysicalDeviceProperties& GetDeviceProperties() const { return g_physicalDeviceProperties; }

		const int32_t GetMemoryTypeIndex(const uint32_t reqMemoryTypeBits, const uint32_t requiredPropertyFlags);
		const bool IsExtensionAvailable(const char* extensionName) const;

	protected:
		void* GetHandleImpl() const override;

	private:
		void FetchMemoryProperties();
		void FetchDeviceProperties();
		void FetchAvailableExtensions();

		VkPhysicalDevice_T* m_physicalDevice = nullptr;

		PhysicalDeviceQueueFamilyIndices m_queueFamilyIndices{};

		Vector<ExtensionProperties> m_availableExtensions;
	};
}
