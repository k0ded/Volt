#include "vkpch.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"
#include "VulkanRHIModule/Graphics/VulkanDebugLayer.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include "VulkanRHIModule/Descriptors/VulkanDescriptorHeap.h"
#include "VulkanRHIModule/Descriptors/ResourceTableDescriptorSetManager.h"
#include "VulkanRHIModule/Pipelines/StaticSamplerDescriptorSetManager.h"

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

	IntRef<GPUAllocator> VulkanGraphicsContext::GetDefaultAllocatorImpl()
	{
		return m_defaultAllocator;
	}

	IntRef<GraphicsDevice> VulkanGraphicsContext::GetGraphicsDevice() const
	{
		return m_graphicsDevice;
	}

	IntRef<PhysicalGraphicsDevice> VulkanGraphicsContext::GetPhysicalGraphicsDevice() const
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

		PhysicalDeviceCreateInfo physicalDeviceCreateInfo{};
		m_physicalDevice = PhysicalGraphicsDevice::Create(physicalDeviceCreateInfo, m_createInfo.enableDebugLayer);
		
		GraphicsDeviceCreateInfo graphicsDeviceInfo{};
		m_graphicsDevice = GraphicsDevice::Create(graphicsDeviceInfo, m_physicalDevice, m_createInfo.enableDebugLayer);
	
		m_defaultAllocator = DefaultGPUAllocator::Create();

		m_resourceTableDescriptorSetManager = CreateRef<ResourceTableDescriptorSetManager>();
		m_staticSamplerDescriptorSetManager = CreateRef<StaticSamplerDescriptorSetManager>();
		m_descriptorHeap = CreateRef<VulkanDescriptorHeap>();

		CreateEmptyDescriptorSetLayout();

		m_pipelineCache.Initialize(m_createInfo.pipelineCacheFilepath);
	}

	void VulkanGraphicsContext::Shutdown()
	{
		m_pipelineCache.Shutdown();

		DestroyEmptyDescriptorSetLayout();
		m_descriptorHeap = nullptr;
		m_staticSamplerDescriptorSetManager = nullptr;
		m_resourceTableDescriptorSetManager = nullptr;

		m_defaultAllocator = nullptr;
		m_transientAllocator = nullptr;

		m_graphicsDevice = nullptr;
		m_physicalDevice = nullptr;

		if (m_debugLayer)
		{
			m_debugLayer->DestroyDebugMessenger(m_instance);
		}

		vkDestroyInstance(m_instance, VT_VULKAN_ALLOCATOR);
	}

	void VulkanGraphicsContext::CreateInstance()
	{
#ifdef VT_ENABLE_VALIDATION
		if (m_createInfo.enableDebugLayer)
		{
			m_debugLayer = CreateRef<VulkanDebugLayer>();

			if (!m_debugLayer->IsSupported())
			{
				VT_LOGC(Warning, LogVulkanRHI, "Vulkan validation layers were requested but not supported. Running without it!");
			}
		}
#endif

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Volt";
		appInfo.pEngineName = "Volt";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_4;

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
		
		VT_VK_CHECK(vkCreateInstance(&createInfo, VT_VULKAN_ALLOCATOR, &m_instance));

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
		extensionsVector.push_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
		extensionsVector.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

		return extensionsVector;
	}

	void VulkanGraphicsContext::CreateEmptyDescriptorSetLayout()
	{
		VkDescriptorSetLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.pNext = nullptr;
		info.bindingCount = 0;
		info.pBindings = nullptr;
		info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

		VT_VK_CHECK(vkCreateDescriptorSetLayout(m_graphicsDevice->GetHandle<VkDevice>(), &info, VT_VULKAN_ALLOCATOR, &m_emptyDescriptorSetLayout));
	}

	void VulkanGraphicsContext::DestroyEmptyDescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout(m_graphicsDevice->GetHandle<VkDevice>(), m_emptyDescriptorSetLayout, VT_VULKAN_ALLOCATOR);
	}
}
