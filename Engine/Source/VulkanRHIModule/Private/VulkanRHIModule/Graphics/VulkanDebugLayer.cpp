#include "vkpch.h"

#include "VulkanRHIModule/Graphics/VulkanDebugLayer.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"

namespace Volt::RHI
{
	static const Vector<VkValidationFeatureEnableEXT> s_enabledValidationFeatures = { /*VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT, /*VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT*/ };
	inline static constexpr std::array<const char*, 1> s_validationLayers = { "VK_LAYER_KHRONOS_validation" };

	inline VkBool32 VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
	{
		const bool isValidation = messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

		if (!isValidation)
		{
			return VK_FALSE;
		}

		std::stringstream sstream;
		sstream << "[";
		sstream << pCallbackData->pMessageIdName;
		sstream << "]\n";
		sstream << pCallbackData->pMessage;

		for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
		{
			sstream << '\n';
			if (pCallbackData->pObjects[i].objectHandle)
			{
				sstream << "	Object Handle [" << i << "] = " << " 0x" << std::hex << pCallbackData->pObjects[i].objectHandle;
			}

			if (pCallbackData->pObjects[i].pObjectName)
			{
				sstream << "[" << pCallbackData->pObjects[i].pObjectName << "]";
			}
		}

		sstream << '\n';

		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				VT_LOG_UNFORMATTED(Trace, sstream.str());
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				VT_LOG_UNFORMATTED(Info, sstream.str());
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				VT_LOG_UNFORMATTED(Warning, sstream.str());
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				VT_LOG_UNFORMATTED(Error, sstream.str());
				break;
		}

		return VK_FALSE;
	}

	VulkanDebugLayer::VulkanDebugLayer()
	{
		CheckValidationLayerSupport();

		if (m_isSupported)
		{
			SetupDebugMessengerInfo();
			SetupValidationFeaturesInfo();
		}
	}

	void VulkanDebugLayer::SetupDebugMessengerInfo()
	{
		m_debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		m_debugMessengerCreateInfo.pNext = nullptr;

		VkDebugUtilsMessageSeverityFlagsEXT severityFlags = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

#ifdef VT_DEBUG
		severityFlags |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
#endif

		m_debugMessengerCreateInfo.messageSeverity = severityFlags;
		m_debugMessengerCreateInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		m_debugMessengerCreateInfo.pfnUserCallback = VulkanDebugCallback;
		m_debugMessengerCreateInfo.pUserData = nullptr;
	}

	void VulkanDebugLayer::SetupValidationFeaturesInfo()
	{
		m_validationFeaturesInfo.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		m_validationFeaturesInfo.pNext = &m_debugMessengerCreateInfo;
		m_validationFeaturesInfo.disabledValidationFeatureCount = 0;
		m_validationFeaturesInfo.pDisabledValidationFeatures = nullptr;
		m_validationFeaturesInfo.enabledValidationFeatureCount = static_cast<uint32_t>(s_enabledValidationFeatures.size());
		m_validationFeaturesInfo.pEnabledValidationFeatures = s_enabledValidationFeatures.data();
	}

	void VulkanDebugLayer::CheckValidationLayerSupport()
	{
		uint32_t layerCount = 0;
		VT_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

		Vector<VkLayerProperties> layerProperties{ layerCount };
		VT_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data()));

		for (const char* layerName : s_validationLayers)
		{
			bool layerFound = false;
			for (const auto& layer : layerProperties)
			{
				if (strcmp(layerName, layer.layerName) != 0)
				{
					layerFound = true;
					break;
				}
			}

			m_isSupported = layerFound;
		}
	}

	void VulkanDebugLayer::SetupCreateInfo(VkInstanceCreateInfo& instanceCreateInfo)
	{
		if (!m_isSupported)
		{
			return;
		}

		if (instanceCreateInfo.pNext)
		{
			m_validationFeaturesInfo.pNext = instanceCreateInfo.pNext;
		}

		instanceCreateInfo.pNext = &m_validationFeaturesInfo;
		instanceCreateInfo.enabledLayerCount = 1;
		instanceCreateInfo.ppEnabledLayerNames = s_validationLayers.data();
	}

	void VulkanDebugLayer::CreateDebugMessenger(VkInstance instance)
	{
		if (m_isSupported)
		{
			vkCreateDebugUtilsMessengerEXT(instance, &m_debugMessengerCreateInfo, nullptr, &m_debugMessenger);
		}
	}

	void VulkanDebugLayer::DestroyDebugMessenger(VkInstance instance)
	{
		if (m_debugMessenger)
		{
			vkDestroyDebugUtilsMessengerEXT(instance, m_debugMessenger, nullptr);
		}
	}
}
