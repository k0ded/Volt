#pragma once

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	class VulkanDebugLayer
	{
	public:
		VulkanDebugLayer();

		void SetupCreateInfo(VkInstanceCreateInfo& instanceCreateInfo);
		void CreateDebugMessenger(VkInstance instance);
		void DestroyDebugMessenger(VkInstance instance);

		VT_NODISCARD VT_INLINE bool IsSupported() const { return m_isSupported; }

	private:
		void SetupDebugMessengerInfo();
		void SetupValidationFeaturesInfo();
		void CheckValidationLayerSupport();

		VkDebugUtilsMessengerCreateInfoEXT m_debugMessengerCreateInfo{};
		VkValidationFeaturesEXT m_validationFeaturesInfo{};
		VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;

		bool m_isSupported = false;
	};
}
