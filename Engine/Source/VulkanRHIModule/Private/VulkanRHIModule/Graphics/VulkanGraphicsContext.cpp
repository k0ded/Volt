#include "vkpch.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"
#include "VulkanRHIModule/Graphics/VulkanDebugLayer.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include "VulkanRHIModule/Memory/VulkanTransientHeap.h"

#include "VulkanRHIModule/Descriptors/VulkanBindlessDescriptorLayoutManager.h"

#include <RHIModule/Graphics/PhysicalGraphicsDevice.h>
#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Memory/GPUAllocator.h>
#include <RHIModule/RHIFeatures.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <array>

namespace Volt::RHI
{
	VulkanGraphicsContext::VulkanGraphicsContext(const GraphicsContextCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		Initialize();
	}

	VulkanGraphicsContext::~VulkanGraphicsContext()
	{
		Shutdown();
	}

	RefPtr<GPUAllocator> VulkanGraphicsContext::GetDefaultAllocatorImpl()
	{
		return m_defaultAllocator;
	}

	RefPtr<GPUAllocator> VulkanGraphicsContext::GetTransientAllocatorImpl()
	{
		return m_transientAllocator;
	}

	RefPtr<ResourceStateTracker> VulkanGraphicsContext::GetResourceStateTrackerImpl()
	{
		return m_resourceStateTracker;
	}

	RefPtr<GraphicsDevice> VulkanGraphicsContext::GetGraphicsDevice() const
	{
		return m_graphicsDevice;
	}

	RefPtr<PhysicalGraphicsDevice> VulkanGraphicsContext::GetPhysicalGraphicsDevice() const
	{
		return m_physicalDevice;
	} 

	void* VulkanGraphicsContext::GetHandleImpl() const
	{
		return m_instance;
	}

	void VulkanGraphicsContext::Initialize()
	{
		CreateInstance();

		m_physicalDevice = PhysicalGraphicsDevice::Create(m_createInfo.physicalDeviceInfo);
		
		GraphicsDeviceCreateInfo graphicsDeviceInfo{};
		graphicsDeviceInfo.physicalDevice = m_physicalDevice;
		m_graphicsDevice = GraphicsDevice::Create(graphicsDeviceInfo);
	
		m_resourceStateTracker = RefPtr<ResourceStateTracker>::Create();
		m_defaultAllocator = DefaultGPUAllocator::Create();
		m_transientAllocator = TransientGPUAllocator::Create();

		if (RHI::RHICanUseBindless())
		{
			VulkanBindlessDescriptorLayoutManager::CreateGlobalDescriptorLayout();
		}
	}

	void VulkanGraphicsContext::Shutdown()
	{
		if (RHI::RHICanUseBindless())
		{
			VulkanBindlessDescriptorLayoutManager::DestroyGlobalDescriptorLayout();
		}

		m_defaultAllocator = nullptr;
		m_transientAllocator = nullptr;

		m_graphicsDevice = nullptr;
		m_physicalDevice = nullptr;

		if (m_debugLayer)
		{
			m_debugLayer->DestroyDebugMessenger(m_instance);
		}

		vkDestroyInstance(m_instance, nullptr);
	}

	void VulkanGraphicsContext::CreateInstance()
	{
#ifdef VT_ENABLE_VALIDATION
		m_debugLayer = CreateRef<VulkanDebugLayer>();

		if (!m_debugLayer->IsSupported())
		{
			VT_LOGC(Warning, LogVulkanRHI, "Vulkan validation layers were requested but not supported. Running without it!");
		}
#endif

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Volt";
		appInfo.pEngineName = "Volt";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		const auto requiredExtensions = GetRequiredExtensions();

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		createInfo.pNext = nullptr;
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;

		if (m_debugLayer)
		{
			m_debugLayer->SetupCreateInfo(createInfo);
		}
		
		VT_VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));

		if (!m_instance)
		{
			throw std::runtime_error("[GraphicsContext] This device does not support Vulkan!");
			return;
		}

		LoadVulkanFunctions(m_instance);

		if (m_debugLayer)
		{
			m_debugLayer->CreateDebugMessenger(m_instance);
		}
	}

	const Vector<const char*> VulkanGraphicsContext::GetRequiredExtensions() const
	{
		uint32_t extensionCount = 0;
		const char** extensions = nullptr;

		extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
		Vector<const char*> extensionsVector{ extensions, extensions + extensionCount };
		extensionsVector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		extensionsVector.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);

		return extensionsVector;
	}
}
