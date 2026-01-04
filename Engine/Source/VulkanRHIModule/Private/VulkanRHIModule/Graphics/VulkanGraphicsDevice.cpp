#include "vkpch.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsDevice.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"	
#include "VulkanRHIModule/Graphics/VulkanDeviceQueue.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/VulkanResourceCast.h"
#include "VulkanRHIModule/Core.h"

#include <RHIModule/RHICapabilities.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	struct EnabledFeatures
	{
		// Native
		VkPhysicalDeviceVulkan11Features vulkan11Features;
		VkPhysicalDeviceVulkan12Features vulkan12Features;
		VkPhysicalDeviceVulkan13Features vulkan13Features;
		VkPhysicalDeviceVulkan14Features vulkan14Features;

		VkPhysicalDeviceFeatures2 physicalDeviceFeatures;
		VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures;

		// Extensions
		VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeaturesEXT{};
		VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeaturesEXT{};
		VkDeviceDiagnosticsConfigCreateInfoNV aftermathDiagInfo{};
		VkPhysicalDeviceRobustness2FeaturesEXT deviceRobustness2FeaturesEXT{};
		VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorTypeFeaturesEXT{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeaturesKHR{};
		VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeaturesKHR{};
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeaturesKHR{};
		VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR physicalDeviceRayTracingMaintenance1FeaturesKHR{};
		VkPhysicalDeviceMaintenance7FeaturesKHR physicalDeviceMaintenance7FeaturesKHR{};
	};

	static EnabledFeatures s_enabledFeatures{};

	namespace Utility
	{
		template<typename T> 
		T GetVulkanExtensionProperties(RawPtr<PhysicalGraphicsDevice> device, VkStructureType type)
		{
			T resultProperties{};
			resultProperties.sType = type;
			resultProperties.pNext = nullptr;

			VkPhysicalDeviceProperties2 deviceProperties{};
			deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			deviceProperties.pNext = &resultProperties;

			vkGetPhysicalDeviceProperties2(device->GetHandle<VkPhysicalDevice>(), &deviceProperties);
			return resultProperties;
		}

		inline static void GetEnabledFeatures(RawPtr<VulkanPhysicalGraphicsDevice> physicalDevice)
		{
			s_enabledFeatures.vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
			s_enabledFeatures.vulkan11Features.pNext = nullptr;
			s_enabledFeatures.vulkan11Features.shaderDrawParameters = VK_TRUE;
			s_enabledFeatures.vulkan11Features.multiview = VK_TRUE;

			s_enabledFeatures.vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			s_enabledFeatures.vulkan12Features.pNext = &s_enabledFeatures.vulkan11Features;
			s_enabledFeatures.vulkan12Features.drawIndirectCount = VK_TRUE;
			s_enabledFeatures.vulkan12Features.samplerFilterMinmax = VK_TRUE;
			s_enabledFeatures.vulkan12Features.hostQueryReset = VK_TRUE;
			s_enabledFeatures.vulkan12Features.runtimeDescriptorArray = VK_TRUE;
			s_enabledFeatures.vulkan12Features.timelineSemaphore = VK_TRUE;

			s_enabledFeatures.vulkan12Features.descriptorIndexing = VK_TRUE;
			s_enabledFeatures.vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
			s_enabledFeatures.vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
			s_enabledFeatures.vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
			s_enabledFeatures.vulkan12Features.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
			s_enabledFeatures.vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;

			s_enabledFeatures.vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
			s_enabledFeatures.vulkan12Features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
			s_enabledFeatures.vulkan12Features.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;

			s_enabledFeatures.vulkan12Features.bufferDeviceAddress = VK_TRUE;
			s_enabledFeatures.vulkan12Features.bufferDeviceAddressCaptureReplay = VK_TRUE;
			s_enabledFeatures.vulkan12Features.shaderBufferInt64Atomics = VK_TRUE;
			s_enabledFeatures.vulkan12Features.shaderSharedInt64Atomics = VK_TRUE;
			s_enabledFeatures.vulkan12Features.shaderFloat16 = VK_FALSE;
			s_enabledFeatures.vulkan12Features.shaderInt8 = VK_FALSE;
			s_enabledFeatures.vulkan12Features.storageBuffer8BitAccess = VK_TRUE;
			s_enabledFeatures.vulkan12Features.scalarBlockLayout = VK_TRUE;
			s_enabledFeatures.vulkan12Features.shaderOutputLayer = VK_TRUE;
			s_enabledFeatures.vulkan12Features.shaderOutputViewportIndex = VK_TRUE;

			s_enabledFeatures.vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
			s_enabledFeatures.vulkan13Features.pNext = &s_enabledFeatures.vulkan12Features;
			s_enabledFeatures.vulkan13Features.synchronization2 = VK_TRUE;
			s_enabledFeatures.vulkan13Features.maintenance4 = VK_TRUE;
			s_enabledFeatures.vulkan13Features.dynamicRendering = VK_TRUE;
			s_enabledFeatures.vulkan13Features.shaderDemoteToHelperInvocation = VK_TRUE;
			s_enabledFeatures.vulkan13Features.subgroupSizeControl = VK_TRUE;

			s_enabledFeatures.vulkan14Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
			s_enabledFeatures.vulkan14Features.pNext = &s_enabledFeatures.vulkan13Features;
			s_enabledFeatures.vulkan14Features.maintenance6 = VK_TRUE;
			s_enabledFeatures.vulkan14Features.pushDescriptor = VK_TRUE;

			void* chainEntryPoint = &s_enabledFeatures.vulkan14Features;

			if (physicalDevice->IsExtensionAvailable(VK_EXT_MESH_SHADER_EXTENSION_NAME) && g_rhiCapabilities.useMeshShaders)
			{
				s_enabledFeatures.meshShaderFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
				s_enabledFeatures.meshShaderFeaturesEXT.meshShader = VK_TRUE;
				s_enabledFeatures.meshShaderFeaturesEXT.taskShader = VK_TRUE;
				s_enabledFeatures.meshShaderFeaturesEXT.pNext = chainEntryPoint;

				chainEntryPoint = &s_enabledFeatures.meshShaderFeaturesEXT;
			}

			if (physicalDevice->IsExtensionAvailable(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME))
			{
				s_enabledFeatures.deviceRobustness2FeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
				s_enabledFeatures.deviceRobustness2FeaturesEXT.pNext = chainEntryPoint;
				s_enabledFeatures.deviceRobustness2FeaturesEXT.nullDescriptor = VK_TRUE;
				s_enabledFeatures.deviceRobustness2FeaturesEXT.robustBufferAccess2 = VK_TRUE;
				s_enabledFeatures.deviceRobustness2FeaturesEXT.robustImageAccess2 = VK_TRUE;
			
				chainEntryPoint = &s_enabledFeatures.deviceRobustness2FeaturesEXT;
			}

			if (physicalDevice->IsExtensionAvailable(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME) && g_rhiCapabilities.useBindless)
			{
				s_enabledFeatures.mutableDescriptorTypeFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
				s_enabledFeatures.mutableDescriptorTypeFeaturesEXT.pNext = chainEntryPoint;
				s_enabledFeatures.mutableDescriptorTypeFeaturesEXT.mutableDescriptorType = VK_TRUE;
				
				chainEntryPoint = &s_enabledFeatures.mutableDescriptorTypeFeaturesEXT;
			}

			if (physicalDevice->IsExtensionAvailable(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) && g_rhiCapabilities.useRayTracing)
			{
				s_enabledFeatures.accelerationStructureFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
				s_enabledFeatures.accelerationStructureFeaturesKHR.pNext = chainEntryPoint;
				s_enabledFeatures.accelerationStructureFeaturesKHR.accelerationStructure = VK_TRUE;
				s_enabledFeatures.accelerationStructureFeaturesKHR.accelerationStructureIndirectBuild = VK_FALSE;
				s_enabledFeatures.accelerationStructureFeaturesKHR.accelerationStructureHostCommands = VK_FALSE;
				s_enabledFeatures.accelerationStructureFeaturesKHR.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
#ifdef VT_DIST
				s_enabledFeatures.accelerationStructureFeaturesKHR.accelerationStructureCaptureReplay = VK_FALSE;
#else
				s_enabledFeatures.accelerationStructureFeaturesKHR.accelerationStructureCaptureReplay = VK_TRUE;
#endif
				chainEntryPoint = &s_enabledFeatures.accelerationStructureFeaturesKHR;
			}

			if (physicalDevice->IsExtensionAvailable(VK_KHR_RAY_QUERY_EXTENSION_NAME) && g_rhiCapabilities.useRayTracing)
			{
				s_enabledFeatures.rayQueryFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
				s_enabledFeatures.rayQueryFeaturesKHR.pNext = chainEntryPoint;
				s_enabledFeatures.rayQueryFeaturesKHR.rayQuery = VK_TRUE;
				chainEntryPoint = &s_enabledFeatures.rayQueryFeaturesKHR;
			}

			if (physicalDevice->IsExtensionAvailable(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) && g_rhiCapabilities.useRayTracing)
			{
				s_enabledFeatures.rayTracingPipelineFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
				s_enabledFeatures.rayTracingPipelineFeaturesKHR.pNext = chainEntryPoint;
				s_enabledFeatures.rayTracingPipelineFeaturesKHR.rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE;
				s_enabledFeatures.rayTracingPipelineFeaturesKHR.rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE;
				s_enabledFeatures.rayTracingPipelineFeaturesKHR.rayTracingPipelineTraceRaysIndirect = VK_FALSE;
				s_enabledFeatures.rayTracingPipelineFeaturesKHR.rayTraversalPrimitiveCulling = VK_FALSE;
				s_enabledFeatures.rayTracingPipelineFeaturesKHR.rayTracingPipeline = VK_TRUE;

				chainEntryPoint = &s_enabledFeatures.rayTracingPipelineFeaturesKHR;
			}

			if (physicalDevice->IsExtensionAvailable(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME) && g_rhiCapabilities.useRayTracing)
			{
				s_enabledFeatures.physicalDeviceRayTracingMaintenance1FeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR;
				s_enabledFeatures.physicalDeviceRayTracingMaintenance1FeaturesKHR.pNext = chainEntryPoint;
				s_enabledFeatures.physicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingMaintenance1 = VK_TRUE;
				s_enabledFeatures.physicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingPipelineTraceRaysIndirect2 = VK_FALSE;
			
				chainEntryPoint = &s_enabledFeatures.physicalDeviceRayTracingMaintenance1FeaturesKHR;
			}

			if (physicalDevice->IsExtensionAvailable(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME))
			{
				s_enabledFeatures.descriptorBufferFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT;
				s_enabledFeatures.descriptorBufferFeaturesEXT.pNext = chainEntryPoint;
				s_enabledFeatures.descriptorBufferFeaturesEXT.descriptorBuffer = VK_TRUE;
				s_enabledFeatures.descriptorBufferFeaturesEXT.descriptorBufferCaptureReplay = VK_TRUE;

				chainEntryPoint = &s_enabledFeatures.descriptorBufferFeaturesEXT;
			}

			if (physicalDevice->IsExtensionAvailable(VK_KHR_MAINTENANCE_7_EXTENSION_NAME))
			{
				s_enabledFeatures.physicalDeviceMaintenance7FeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_7_FEATURES_KHR;
				s_enabledFeatures.physicalDeviceMaintenance7FeaturesKHR.pNext = chainEntryPoint;
				s_enabledFeatures.physicalDeviceMaintenance7FeaturesKHR.maintenance7 = VK_TRUE;

				chainEntryPoint = &s_enabledFeatures.physicalDeviceMaintenance7FeaturesKHR;
			}

#ifdef VT_ENABLE_NV_AFTERMATH
			s_enabledFeatures.aftermathDiagInfo.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
			s_enabledFeatures.aftermathDiagInfo.pNext = chainEntryPoint;
			s_enabledFeatures.aftermathDiagInfo.flags =
				VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
				VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |
				VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV |
				VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_ERROR_REPORTING_BIT_NV; // #TODO_Ivar: Implement some kind of debug level

			chainEntryPoint = &s_enabledFeatures.aftermathDiagInfo;
#endif

			s_enabledFeatures.physicalDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			s_enabledFeatures.physicalDeviceFeatures.pNext = chainEntryPoint;
			s_enabledFeatures.physicalDeviceFeatures.features.inheritedQueries = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.multiDrawIndirect = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.geometryShader = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.imageCubeArray = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.samplerAnisotropy = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.pipelineStatisticsQuery = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.fillModeNonSolid = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.wideLines = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.independentBlend = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.depthClamp = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.depthBiasClamp = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.shaderImageGatherExtended = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.robustBufferAccess = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.shaderStorageImageArrayDynamicIndexing = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.vertexPipelineStoresAndAtomics = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.fragmentStoresAndAtomics = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.sampleRateShading = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
			s_enabledFeatures.physicalDeviceFeatures.features.shaderStorageImageReadWithoutFormat = VK_TRUE;

			s_enabledFeatures.physicalDeviceFeatures.features.shaderInt16 = VK_FALSE; // #TODO_Ivar: does not work on older cards
		}

		inline static Vector<const char*> GetEnabledExtensions(RawPtr<VulkanPhysicalGraphicsDevice> physicalDevice)
		{
			Vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

			if (physicalDevice->IsExtensionAvailable(VK_EXT_MESH_SHADER_EXTENSION_NAME) && g_rhiCapabilities.useMeshShaders)
			{
				enabledExtensions.emplace_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
			}

			if (physicalDevice->IsExtensionAvailable(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME))
			{
				enabledExtensions.emplace_back(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME);
			}

			if (physicalDevice->IsExtensionAvailable(VK_KHR_RAY_QUERY_EXTENSION_NAME) && g_rhiCapabilities.useRayTracing)
			{
				enabledExtensions.emplace_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
			}

			if (physicalDevice->IsExtensionAvailable(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) && g_rhiCapabilities.useRayTracing)
			{
				enabledExtensions.emplace_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
			}

			if (physicalDevice->IsExtensionAvailable(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME) && g_rhiCapabilities.useRayTracing)
			{
				enabledExtensions.emplace_back(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME);
			}

			if (physicalDevice->IsExtensionAvailable(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) && physicalDevice->IsExtensionAvailable(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) && g_rhiCapabilities.useRayTracing)
			{
				enabledExtensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
				enabledExtensions.emplace_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
			}

#ifdef VT_ENABLE_NV_AFTERMATH
			if (physicalDevice->IsExtensionAvailable(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME))
			{
				enabledExtensions.emplace_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
			}

			if (physicalDevice->IsExtensionAvailable(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME))
			{
				enabledExtensions.emplace_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);
			}
#endif

#ifdef VT_ENABLE_VALIDATION
			if (physicalDevice->IsExtensionAvailable(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
			{
				enabledExtensions.emplace_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
			}
#endif

			if (physicalDevice->IsExtensionAvailable(VK_NV_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME))
			{
				enabledExtensions.emplace_back(VK_NV_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME);
			}

			if (physicalDevice->IsExtensionAvailable(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME) && g_rhiCapabilities.useBindless)
			{
				enabledExtensions.emplace_back(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME);
			}

			if (physicalDevice->IsExtensionAvailable(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME))
			{
				enabledExtensions.emplace_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
			}

			if (physicalDevice->IsExtensionAvailable(VK_EXT_DEVICE_FAULT_EXTENSION_NAME))
			{
				enabledExtensions.emplace_back(VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
			}

			if (physicalDevice->IsExtensionAvailable(VK_KHR_MAINTENANCE_7_EXTENSION_NAME))
			{
				enabledExtensions.emplace_back(VK_KHR_MAINTENANCE_7_EXTENSION_NAME);
			}

			return enabledExtensions;
		}
	}

	inline static const char* s_validationLayer = "VK_LAYER_KHRONOS_validation";

	VulkanGraphicsDevice::VulkanGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer)
	{
		m_physicalDevice = physicalGraphicsDevice.As<VulkanPhysicalGraphicsDevice>();

		InitializeCapabilities();

		VulkanPhysicalGraphicsDevice& physicalDevicePtr = m_physicalDevice->AsRef<VulkanPhysicalGraphicsDevice>();
		const auto& queueFamilies = physicalDevicePtr.GetQueueFamilies();

		std::array<VkDeviceQueueCreateInfo, 3> deviceQueueInfos{};
		constexpr std::array<float, 3> queuePriorities = { 1.f, 1.f, 1.f };

		// Graphics Queue
		{
			auto& queueCreateInfo = deviceQueueInfos[0];
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.pQueuePriorities = queuePriorities.data();
			queueCreateInfo.queueFamilyIndex = queueFamilies.graphicsFamilyQueueIndex;
			queueCreateInfo.queueCount = 1;
		}

		// Compute Queue
		{
			auto& queueCreateInfo = deviceQueueInfos[1];
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.pQueuePriorities = queuePriorities.data();
			queueCreateInfo.queueFamilyIndex = queueFamilies.computeFamilyQueueIndex;
			queueCreateInfo.queueCount = 1;
		}

		// Transfer Queue
		{
			auto& queueCreateInfo = deviceQueueInfos[2];
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.pQueuePriorities = queuePriorities.data();
			queueCreateInfo.queueFamilyIndex = queueFamilies.transferFamilyQueueIndex;
			queueCreateInfo.queueCount = 1;
		}

		// Create Device
		{
			VkDeviceCreateInfo deviceInfo{};
			deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueInfos.size());
			deviceInfo.pQueueCreateInfos = deviceQueueInfos.data();
			deviceInfo.pEnabledFeatures = nullptr;
			deviceInfo.enabledLayerCount = 0;

#ifdef VT_ENABLE_VALIDATION
			if (enableDebugLayer)
			{
				deviceInfo.enabledLayerCount = 1u;
				deviceInfo.ppEnabledLayerNames = &s_validationLayer;
			}
#endif

			//VT_ENSURE_MSG(m_physicalDevice->IsExtensionAvailable(VK_EXT_MESH_SHADER_EXTENSION_NAME), "Mesh Shader support is required!");
			//VT_ENSURE_MSG(m_physicalDevice->IsExtensionAvailable(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME), "Mutable descriptor type support is required!");

			const auto enabledExtensions = Utility::GetEnabledExtensions(m_physicalDevice);
			Utility::GetEnabledFeatures(m_physicalDevice);

			deviceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
			deviceInfo.ppEnabledExtensionNames = enabledExtensions.data();
			deviceInfo.pNext = &s_enabledFeatures.physicalDeviceFeatures;

#ifdef VT_ENABLE_NV_AFTERMATH
			m_deviceCrashTracker.Initialize(GraphicsAPI::Vulkan);
#endif

			VT_VK_CHECK(vkCreateDevice(physicalDevicePtr.GetHandle<VkPhysicalDevice>(), &deviceInfo, VT_VULKAN_ALLOCATOR, &m_device));
		}

		m_deviceQueues[QueueType::Graphics] = RefPtr<VulkanDeviceQueue>::Create(DeviceQueueCreateInfo{ this, QueueType::Graphics });
		m_deviceQueues[QueueType::TransferCopy] = RefPtr<VulkanDeviceQueue>::Create(DeviceQueueCreateInfo{ this, QueueType::TransferCopy });
		m_deviceQueues[QueueType::Compute] = RefPtr<VulkanDeviceQueue>::Create(DeviceQueueCreateInfo{ this, QueueType::Compute });

#ifdef VT_ENABLE_GPU_PROFILING
		{
			VkPhysicalDevice physicalDevice = m_physicalDevice.lock()->GetHandle<VkPhysicalDevice>();
			VkQueue graphicsQueue = m_deviceQueues[QueueType::Graphics]->GetHandle<VkQueue>();

			uint32_t graphicsFamily = static_cast<uint32_t>(queueFamilies.graphicsFamilyQueueIndex);

			OPTICK_GPU_INIT_VULKAN(&m_device, &physicalDevice, &graphicsQueue, &graphicsFamily, 1, nullptr);
		}
#endif
	}

	VulkanGraphicsDevice::~VulkanGraphicsDevice()
	{
		for (auto& [queueType, queue] : m_deviceQueues)
		{
			queue->AsRef<VulkanDeviceQueue>().DestroyQueueSemaphore(*this);
		}

		vkDestroyDevice(m_device, VT_VULKAN_ALLOCATOR);
	}

	void VulkanGraphicsDevice::WaitForIdle()
	{
		RefPtr<VulkanDeviceQueue> graphicsQueue = ResourceCast(m_deviceQueues[QueueType::Graphics]);
		RefPtr<VulkanDeviceQueue> transferQueue = ResourceCast(m_deviceQueues[QueueType::TransferCopy]);
		RefPtr<VulkanDeviceQueue> computeQueue = ResourceCast(m_deviceQueues[QueueType::Compute]);

		graphicsQueue->AquireLock();
		transferQueue->AquireLock();
		computeQueue->AquireLock();

		vkDeviceWaitIdle(m_device);

		computeQueue->ReleaseLock();
		transferQueue->ReleaseLock();
		graphicsQueue->ReleaseLock();
	}

	RefPtr<DeviceQueue> VulkanGraphicsDevice::GetDeviceQueue(QueueType queueType) const
	{
		return m_deviceQueues.at(queueType);
	}

	RawPtr<VulkanPhysicalGraphicsDevice> VulkanGraphicsDevice::GetPhysicalDevice() const
	{
		return m_physicalDevice;
	}

	void* VulkanGraphicsDevice::GetHandleImpl() const
	{
		return m_device;
	}

	void VulkanGraphicsDevice::InitializeCapabilities()
	{
		const auto& deviceProperties = m_physicalDevice->GetDeviceProperties();
	
		g_rhiCapabilities.max2DTextureDimensions = deviceProperties.limits.maxImageDimension2D;
		g_rhiCapabilities.maxBufferDimensions = deviceProperties.limits.maxTexelBufferElements;
		g_rhiCapabilities.max3DTextureDimensions = deviceProperties.limits.maxImageDimension3D;
		g_rhiCapabilities.maxCubeTextureDimensions = deviceProperties.limits.maxImageDimensionCube;
		g_rhiCapabilities.maxTextureArrayLayers = deviceProperties.limits.maxImageArrayLayers;
		g_rhiCapabilities.maxTextureSamplers = deviceProperties.limits.maxDescriptorSetSamplers;
		g_rhiCapabilities.maxComputeSharedMemorySize = deviceProperties.limits.maxComputeSharedMemorySize;
		g_rhiCapabilities.maxWorkGroupInvocations = deviceProperties.limits.maxComputeWorkGroupInvocations;
		g_rhiCapabilities.minUniformBufferAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;

		g_rhiCapabilities.rayTracing.supportsRayTracing = m_physicalDevice->IsExtensionAvailable(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) && m_physicalDevice->IsExtensionAvailable(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		g_rhiCapabilities.rayTracing.supportsInlineRaytracing = m_physicalDevice->IsExtensionAvailable(VK_KHR_RAY_QUERY_EXTENSION_NAME);
		g_rhiCapabilities.rayTracing.supportsRaytracingShaders = m_physicalDevice->IsExtensionAvailable(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		g_rhiCapabilities.rayTracing.accelerationStructureAlignment = 256;
		g_rhiCapabilities.rayTracing.scratchBufferAlignment = 256;

		{
			auto properties = Utility::GetVulkanExtensionProperties<VkPhysicalDeviceSubgroupSizeControlProperties>(m_physicalDevice, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES);
			g_rhiCapabilities.minimumWaveSize = properties.minSubgroupSize;
			g_rhiCapabilities.maximumWaveSize = properties.maxSubgroupSize;
		}

		g_rhiCapabilities.supportsMeshShaders = m_physicalDevice->IsExtensionAvailable(VK_EXT_MESH_SHADER_EXTENSION_NAME);
		g_rhiCapabilities.supportsBindless = m_physicalDevice->IsExtensionAvailable(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME);
		g_rhiCapabilities.supportsNative16BitOperations = false;
		g_rhiCapabilities.maxDispatchThreadGroupsPerDimension.x = deviceProperties.limits.maxComputeWorkGroupCount[0];
		g_rhiCapabilities.maxDispatchThreadGroupsPerDimension.y = deviceProperties.limits.maxComputeWorkGroupCount[1];
		g_rhiCapabilities.maxDispatchThreadGroupsPerDimension.z = deviceProperties.limits.maxComputeWorkGroupCount[2];
	}

	uint64_t VulkanGraphicsDevice::GetMaxRequiredStagingBufferSizeForImage(RawPtr<Image> image) const
	{
		const ImageDesc& desc = image->GetDesc();

		const uint32_t formatTexelBlockSize = RHI::Utility::GetFormatTexelBlockSize(desc.format);
		const uint32_t formatTexelsPerBlock = RHI::Utility::GetFormatTexelsPerBlock(desc.format);

		uint64_t totalImageSize = 0;

		for (uint32_t i = 0; i < image->GetMipCount(); ++i)
		{
			const uint32_t width = std::max(image->GetWidth() >> i, 1u);
			const uint32_t height = std::max(image->GetHeight() >> i, 1u);

			const uint32_t blockTexelSize = static_cast<uint32_t>(sqrt(formatTexelsPerBlock));
			const uint32_t blockWidth = std::max(1u, (width + blockTexelSize - 1u) / blockTexelSize);
			const uint32_t blockHeight = std::max(1u, (height + blockTexelSize - 1u) / blockTexelSize);

			const uint64_t mipSize = blockWidth * blockHeight * formatTexelBlockSize * desc.layers;

			totalImageSize += mipSize;
		}

		return totalImageSize;
	}

	uint64_t VulkanGraphicsDevice::GetRowPitchForWidth(RawPtr<Image> image, uint32_t width) const
	{
		const ImageDesc& desc = image->GetDesc();
		return width * RHI::Utility::GetByteSizePerPixelFromFormat(desc.format);
	}
}

