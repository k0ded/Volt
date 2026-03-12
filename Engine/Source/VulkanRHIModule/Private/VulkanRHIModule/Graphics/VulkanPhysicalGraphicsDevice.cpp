#include "vkpch.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	namespace Utility
	{
		const VkPhysicalDevice FindBestSuitableDevice(VkInstance instance)
		{
			uint32_t deviceCount = 0;
			VT_VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

			if (deviceCount == 0)
			{
				throw std::runtime_error("[PhysicalDevice] Failed to find GPU with Vulkan support!");
			}

			Vector<VkPhysicalDevice> devices{ deviceCount };
			VT_VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

			VkPhysicalDevice nonDiscreteDevice = nullptr;

			VkPhysicalDeviceProperties deviceProperties{};
			for (const auto& device : devices)
			{
				vkGetPhysicalDeviceProperties(device, &deviceProperties);

				constexpr auto VERSION = VK_API_VERSION_1_4;

				if (deviceProperties.apiVersion >= VERSION)
				{
					if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
					{
						return device;
					}
				
					nonDiscreteDevice = device;
				}
			}

			if (!nonDiscreteDevice)
			{
				throw std::runtime_error("[PhysicalDevice] Failed to find GPU with Vulkan 1.3 support!");
			}

			return nonDiscreteDevice;
		}

		const PhysicalDeviceQueueFamilyIndices FindQueueFamilyIndices(VkPhysicalDevice physicalDevice)
		{
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

			Vector<VkQueueFamilyProperties> queueFamilyProperties{ queueFamilyCount };
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

			PhysicalDeviceQueueFamilyIndices result{};

			for (int32_t i = 0; const auto& queueFamily : queueFamilyProperties)
			{
				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && result.graphicsFamilyQueueIndex == -1)
				{
					result.graphicsFamilyQueueIndex = i;
					i++;
					continue;
				}

				if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT && result.computeFamilyQueueIndex == -1)
				{
					result.computeFamilyQueueIndex = i;
					i++;
					continue;
				}

				if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && result.transferFamilyQueueIndex== -1)
				{
					result.transferFamilyQueueIndex = i;
					i++;
					continue;
				}

				if (result.computeFamilyQueueIndex != -1 &&
					result.graphicsFamilyQueueIndex != -1 &&
					result.transferFamilyQueueIndex != -1)
				{
					break;
				}

				i++;
			}

			if (result.computeFamilyQueueIndex == -1)
			{
				result.computeFamilyQueueIndex = result.graphicsFamilyQueueIndex;
			}

			if (result.transferFamilyQueueIndex == -1)
			{
				result.transferFamilyQueueIndex = result.graphicsFamilyQueueIndex;
			}

			return result;
		}
	}

	VulkanPhysicalGraphicsDevice::VulkanPhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer)
	{
		VkInstance vulkanInstance = GraphicsContext::Get().AsRef<VulkanGraphicsContext>().GetHandle<VkInstance>();
		VkPhysicalDevice selectedDevice = Utility::FindBestSuitableDevice(vulkanInstance);

		m_physicalDevice = selectedDevice;
		m_queueFamilyIndices = Utility::FindQueueFamilyIndices(selectedDevice);

		FetchAvailableExtensions();
		FetchDeviceProperties();
	}

	VulkanPhysicalGraphicsDevice::~VulkanPhysicalGraphicsDevice()
	{
	}

	const int32_t VulkanPhysicalGraphicsDevice::GetMemoryTypeIndex(const uint32_t reqMemoryTypeBits, const uint32_t requiredPropertyFlags)
	{
		for (const auto& memoryType : g_physicalDeviceProperties.memoryProperties.memoryTypes)
		{
			const uint32_t memTypeBits = (1 << memoryType.index);
			const bool isRequiredMemoryType = reqMemoryTypeBits & memTypeBits;
		
			const VkMemoryPropertyFlags properties = static_cast<VkMemoryPropertyFlags>(memoryType.propertyFlags);
			const bool hasRequiredProperties = (properties & requiredPropertyFlags) == requiredPropertyFlags;

			if (isRequiredMemoryType && hasRequiredProperties)
			{
				return static_cast<int32_t>(memoryType.index);
			}
		}

		return -1;
	}

	const bool VulkanPhysicalGraphicsDevice::IsExtensionAvailable(const char* extensionName) const
	{
		for (const auto& ext : m_availableExtensions)
		{
			if (strcmp(extensionName, ext.extensionName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	void* VulkanPhysicalGraphicsDevice::GetHandleImpl() const
	{
		return m_physicalDevice;
	}

	void VulkanPhysicalGraphicsDevice::FetchMemoryProperties()
	{
		VkPhysicalDeviceMemoryProperties memoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			auto& memType = g_physicalDeviceProperties.memoryProperties.memoryTypes.emplace_back();
			memType.heapIndex = memoryProperties.memoryTypes[i].heapIndex;
			memType.propertyFlags = memoryProperties.memoryTypes[i].propertyFlags;
			memType.index = i;
		}

		for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++)
		{
			auto& memHeap = g_physicalDeviceProperties.memoryProperties.memoryHeaps.emplace_back();
			memHeap.flags = memoryProperties.memoryHeaps[i].flags;
			memHeap.size = memoryProperties.memoryHeaps[i].size;
		}
	}

	void VulkanPhysicalGraphicsDevice::FetchDeviceProperties()
	{
		void* firstChainPtr = nullptr;

		VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferProperties{};
		descriptorBufferProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
		descriptorBufferProperties.pNext = firstChainPtr;
		firstChainPtr = &descriptorBufferProperties;

		VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderProperties{};
		meshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
		meshShaderProperties.pNext = firstChainPtr;
		firstChainPtr = &meshShaderProperties;

		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
		rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		rayTracingPipelineProperties.pNext = firstChainPtr;
		firstChainPtr = &rayTracingPipelineProperties;

		VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{};
		accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
		accelerationStructureProperties.pNext = firstChainPtr;
		firstChainPtr = &accelerationStructureProperties;

		VkPhysicalDeviceProperties2	deviceProperties{};
		deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties.pNext = firstChainPtr;

		vkGetPhysicalDeviceProperties2(m_physicalDevice, &deviceProperties);

		g_physicalDeviceProperties.deviceName = deviceProperties.properties.deviceName;
		g_physicalDeviceProperties.vendor = VendorIDToVendor(deviceProperties.properties.vendorID);

		// Limits
		{
			g_physicalDeviceProperties.limits.maxImageDimension1D = deviceProperties.properties.limits.maxImageDimension1D;
			g_physicalDeviceProperties.limits.maxImageDimension2D = deviceProperties.properties.limits.maxImageDimension2D;
			g_physicalDeviceProperties.limits.maxImageDimension3D = deviceProperties.properties.limits.maxImageDimension3D;
			g_physicalDeviceProperties.limits.maxImageDimensionCube = deviceProperties.properties.limits.maxImageDimensionCube;
			g_physicalDeviceProperties.limits.maxImageArrayLayers = deviceProperties.properties.limits.maxImageArrayLayers;
			g_physicalDeviceProperties.limits.maxTexelBufferElements = deviceProperties.properties.limits.maxTexelBufferElements;
			g_physicalDeviceProperties.limits.maxUniformBufferRange = deviceProperties.properties.limits.maxUniformBufferRange;
			g_physicalDeviceProperties.limits.maxStorageBufferRange = deviceProperties.properties.limits.maxStorageBufferRange;
			g_physicalDeviceProperties.limits.maxPushConstantsSize = deviceProperties.properties.limits.maxPushConstantsSize;
			g_physicalDeviceProperties.limits.maxMemoryAllocationCount = deviceProperties.properties.limits.maxMemoryAllocationCount;
			g_physicalDeviceProperties.limits.maxSamplerAllocationCount = deviceProperties.properties.limits.maxSamplerAllocationCount;
			g_physicalDeviceProperties.limits.bufferImageGranularity = deviceProperties.properties.limits.bufferImageGranularity;
			g_physicalDeviceProperties.limits.sparseAddressSpaceSize = deviceProperties.properties.limits.sparseAddressSpaceSize;
			g_physicalDeviceProperties.limits.maxBoundDescriptorSets = deviceProperties.properties.limits.maxBoundDescriptorSets;
			g_physicalDeviceProperties.limits.maxPerStageDescriptorSamplers = deviceProperties.properties.limits.maxPerStageDescriptorSamplers;
			g_physicalDeviceProperties.limits.maxPerStageDescriptorUniformBuffers = deviceProperties.properties.limits.maxPerStageDescriptorUniformBuffers;
			g_physicalDeviceProperties.limits.maxPerStageDescriptorStorageBuffers = deviceProperties.properties.limits.maxPerStageDescriptorStorageBuffers;
			g_physicalDeviceProperties.limits.maxPerStageDescriptorSampledImages = deviceProperties.properties.limits.maxPerStageDescriptorSampledImages;
			g_physicalDeviceProperties.limits.maxPerStageDescriptorStorageImages = deviceProperties.properties.limits.maxPerStageDescriptorStorageImages;
			g_physicalDeviceProperties.limits.maxPerStageDescriptorInputAttachments = deviceProperties.properties.limits.maxPerStageDescriptorInputAttachments;
			g_physicalDeviceProperties.limits.maxPerStageResources = deviceProperties.properties.limits.maxPerStageResources;
			g_physicalDeviceProperties.limits.maxDescriptorSetSamplers = deviceProperties.properties.limits.maxDescriptorSetSamplers;
			g_physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers = deviceProperties.properties.limits.maxDescriptorSetUniformBuffers;
			g_physicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic = deviceProperties.properties.limits.maxDescriptorSetUniformBuffersDynamic;
			g_physicalDeviceProperties.limits.maxDescriptorSetStorageBuffers = deviceProperties.properties.limits.maxDescriptorSetStorageBuffers;
			g_physicalDeviceProperties.limits.maxDescriptorSetStorageBuffersDynamic = deviceProperties.properties.limits.maxDescriptorSetStorageBuffersDynamic;
			g_physicalDeviceProperties.limits.maxDescriptorSetSampledImages = deviceProperties.properties.limits.maxDescriptorSetSampledImages;
			g_physicalDeviceProperties.limits.maxDescriptorSetStorageImages = deviceProperties.properties.limits.maxDescriptorSetStorageImages;
			g_physicalDeviceProperties.limits.maxDescriptorSetInputAttachments = deviceProperties.properties.limits.maxDescriptorSetInputAttachments;
			g_physicalDeviceProperties.limits.maxVertexInputAttributes = deviceProperties.properties.limits.maxVertexInputAttributes;
			g_physicalDeviceProperties.limits.maxVertexInputBindings = deviceProperties.properties.limits.maxVertexInputBindings;
			g_physicalDeviceProperties.limits.maxVertexInputAttributeOffset = deviceProperties.properties.limits.maxVertexInputAttributeOffset;
			g_physicalDeviceProperties.limits.maxVertexInputBindingStride = deviceProperties.properties.limits.maxVertexInputBindingStride;
			g_physicalDeviceProperties.limits.maxVertexOutputComponents = deviceProperties.properties.limits.maxVertexOutputComponents;
			g_physicalDeviceProperties.limits.maxTessellationGenerationLevel = deviceProperties.properties.limits.maxTessellationGenerationLevel;
			g_physicalDeviceProperties.limits.maxTessellationPatchSize = deviceProperties.properties.limits.maxTessellationPatchSize;
			g_physicalDeviceProperties.limits.maxTessellationControlPerVertexInputComponents = deviceProperties.properties.limits.maxTessellationControlPerVertexInputComponents;
			g_physicalDeviceProperties.limits.maxTessellationControlPerVertexOutputComponents = deviceProperties.properties.limits.maxTessellationControlPerVertexOutputComponents;
			g_physicalDeviceProperties.limits.maxTessellationControlPerPatchOutputComponents = deviceProperties.properties.limits.maxTessellationControlPerPatchOutputComponents;
			g_physicalDeviceProperties.limits.maxTessellationControlTotalOutputComponents = deviceProperties.properties.limits.maxTessellationControlTotalOutputComponents;
			g_physicalDeviceProperties.limits.maxTessellationEvaluationInputComponents = deviceProperties.properties.limits.maxTessellationEvaluationInputComponents;
			g_physicalDeviceProperties.limits.maxTessellationEvaluationOutputComponents = deviceProperties.properties.limits.maxTessellationEvaluationOutputComponents;
			g_physicalDeviceProperties.limits.maxGeometryShaderInvocations = deviceProperties.properties.limits.maxGeometryShaderInvocations;
			g_physicalDeviceProperties.limits.maxGeometryInputComponents = deviceProperties.properties.limits.maxGeometryInputComponents;
			g_physicalDeviceProperties.limits.maxGeometryOutputComponents = deviceProperties.properties.limits.maxGeometryOutputComponents;
			g_physicalDeviceProperties.limits.maxGeometryOutputVertices = deviceProperties.properties.limits.maxGeometryOutputVertices;
			g_physicalDeviceProperties.limits.maxGeometryTotalOutputComponents = deviceProperties.properties.limits.maxGeometryTotalOutputComponents;
			g_physicalDeviceProperties.limits.maxFragmentInputComponents = deviceProperties.properties.limits.maxFragmentInputComponents;
			g_physicalDeviceProperties.limits.maxFragmentOutputAttachments = deviceProperties.properties.limits.maxFragmentOutputAttachments;
			g_physicalDeviceProperties.limits.maxFragmentDualSrcAttachments = deviceProperties.properties.limits.maxFragmentDualSrcAttachments;
			g_physicalDeviceProperties.limits.maxFragmentCombinedOutputResources = deviceProperties.properties.limits.maxFragmentCombinedOutputResources;
			g_physicalDeviceProperties.limits.maxComputeSharedMemorySize = deviceProperties.properties.limits.maxComputeSharedMemorySize;
			g_physicalDeviceProperties.limits.maxComputeWorkGroupCount[0] = deviceProperties.properties.limits.maxComputeWorkGroupCount[0];
			g_physicalDeviceProperties.limits.maxComputeWorkGroupCount[1] = deviceProperties.properties.limits.maxComputeWorkGroupCount[1];
			g_physicalDeviceProperties.limits.maxComputeWorkGroupCount[2] = deviceProperties.properties.limits.maxComputeWorkGroupCount[2];
			g_physicalDeviceProperties.limits.maxComputeWorkGroupInvocations = deviceProperties.properties.limits.maxComputeWorkGroupInvocations;
			g_physicalDeviceProperties.limits.maxComputeWorkGroupSize[0] = deviceProperties.properties.limits.maxComputeWorkGroupSize[0];
			g_physicalDeviceProperties.limits.maxComputeWorkGroupSize[1] = deviceProperties.properties.limits.maxComputeWorkGroupSize[1];
			g_physicalDeviceProperties.limits.maxComputeWorkGroupSize[2] = deviceProperties.properties.limits.maxComputeWorkGroupSize[2];
			g_physicalDeviceProperties.limits.subPixelPrecisionBits = deviceProperties.properties.limits.subPixelPrecisionBits;
			g_physicalDeviceProperties.limits.subTexelPrecisionBits = deviceProperties.properties.limits.subTexelPrecisionBits;
			g_physicalDeviceProperties.limits.mipmapPrecisionBits = deviceProperties.properties.limits.mipmapPrecisionBits;
			g_physicalDeviceProperties.limits.maxDrawIndexedIndexValue = deviceProperties.properties.limits.maxDrawIndexedIndexValue;
			g_physicalDeviceProperties.limits.maxDrawIndirectCount = deviceProperties.properties.limits.maxDrawIndirectCount;
			g_physicalDeviceProperties.limits.maxSamplerLodBias = deviceProperties.properties.limits.maxSamplerLodBias;
			g_physicalDeviceProperties.limits.maxSamplerAnisotropy = deviceProperties.properties.limits.maxSamplerAnisotropy;
			g_physicalDeviceProperties.limits.maxViewports = deviceProperties.properties.limits.maxViewports;
			g_physicalDeviceProperties.limits.maxViewportDimensions[0] = deviceProperties.properties.limits.maxViewportDimensions[0];
			g_physicalDeviceProperties.limits.maxViewportDimensions[1] = deviceProperties.properties.limits.maxViewportDimensions[1];
			g_physicalDeviceProperties.limits.viewportBoundsRange[0] = deviceProperties.properties.limits.viewportBoundsRange[0];
			g_physicalDeviceProperties.limits.viewportBoundsRange[1] = deviceProperties.properties.limits.viewportBoundsRange[1];
			g_physicalDeviceProperties.limits.viewportSubPixelBits = deviceProperties.properties.limits.viewportSubPixelBits;
			g_physicalDeviceProperties.limits.minMemoryMapAlignment = deviceProperties.properties.limits.minMemoryMapAlignment;
			g_physicalDeviceProperties.limits.minTexelBufferOffsetAlignment = deviceProperties.properties.limits.minTexelBufferOffsetAlignment;
			g_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment = deviceProperties.properties.limits.minUniformBufferOffsetAlignment;
			g_physicalDeviceProperties.limits.minStorageBufferOffsetAlignment = deviceProperties.properties.limits.minStorageBufferOffsetAlignment;
			g_physicalDeviceProperties.limits.minTexelOffset = deviceProperties.properties.limits.minTexelOffset;
			g_physicalDeviceProperties.limits.maxTexelOffset = deviceProperties.properties.limits.maxTexelOffset;
			g_physicalDeviceProperties.limits.minTexelGatherOffset = deviceProperties.properties.limits.minTexelGatherOffset;
			g_physicalDeviceProperties.limits.maxTexelGatherOffset = deviceProperties.properties.limits.maxTexelGatherOffset;
			g_physicalDeviceProperties.limits.minInterpolationOffset = deviceProperties.properties.limits.minInterpolationOffset;
			g_physicalDeviceProperties.limits.maxInterpolationOffset = deviceProperties.properties.limits.maxInterpolationOffset;
			g_physicalDeviceProperties.limits.subPixelInterpolationOffsetBits = deviceProperties.properties.limits.subPixelInterpolationOffsetBits;
			g_physicalDeviceProperties.limits.maxFramebufferWidth = deviceProperties.properties.limits.maxFramebufferWidth;
			g_physicalDeviceProperties.limits.maxFramebufferHeight = deviceProperties.properties.limits.maxFramebufferHeight;
			g_physicalDeviceProperties.limits.maxFramebufferLayers = deviceProperties.properties.limits.maxFramebufferLayers;
			g_physicalDeviceProperties.limits.framebufferColorSampleCounts = deviceProperties.properties.limits.framebufferColorSampleCounts;
			g_physicalDeviceProperties.limits.framebufferDepthSampleCounts = deviceProperties.properties.limits.framebufferDepthSampleCounts;
			g_physicalDeviceProperties.limits.framebufferStencilSampleCounts = deviceProperties.properties.limits.framebufferStencilSampleCounts;
			g_physicalDeviceProperties.limits.framebufferNoAttachmentsSampleCounts = deviceProperties.properties.limits.framebufferNoAttachmentsSampleCounts;
			g_physicalDeviceProperties.limits.maxColorAttachments = deviceProperties.properties.limits.maxColorAttachments;
			g_physicalDeviceProperties.limits.sampledImageColorSampleCounts = deviceProperties.properties.limits.sampledImageColorSampleCounts;
			g_physicalDeviceProperties.limits.sampledImageIntegerSampleCounts = deviceProperties.properties.limits.sampledImageIntegerSampleCounts;
			g_physicalDeviceProperties.limits.sampledImageDepthSampleCounts = deviceProperties.properties.limits.sampledImageDepthSampleCounts;
			g_physicalDeviceProperties.limits.sampledImageStencilSampleCounts = deviceProperties.properties.limits.sampledImageStencilSampleCounts;
			g_physicalDeviceProperties.limits.storageImageSampleCounts = deviceProperties.properties.limits.storageImageSampleCounts;
			g_physicalDeviceProperties.limits.maxSampleMaskWords = deviceProperties.properties.limits.maxSampleMaskWords;
			g_physicalDeviceProperties.limits.timestampComputeAndGraphics = deviceProperties.properties.limits.timestampComputeAndGraphics;
			g_physicalDeviceProperties.limits.timestampPeriod = deviceProperties.properties.limits.timestampPeriod;
			g_physicalDeviceProperties.limits.maxClipDistances = deviceProperties.properties.limits.maxClipDistances;
			g_physicalDeviceProperties.limits.maxCullDistances = deviceProperties.properties.limits.maxCullDistances;
			g_physicalDeviceProperties.limits.maxCombinedClipAndCullDistances = deviceProperties.properties.limits.maxCombinedClipAndCullDistances;
			g_physicalDeviceProperties.limits.discreteQueuePriorities = deviceProperties.properties.limits.discreteQueuePriorities;
			g_physicalDeviceProperties.limits.pointSizeRange[0] = deviceProperties.properties.limits.pointSizeRange[0];
			g_physicalDeviceProperties.limits.pointSizeRange[1] = deviceProperties.properties.limits.pointSizeRange[1];
			g_physicalDeviceProperties.limits.lineWidthRange[0] = deviceProperties.properties.limits.lineWidthRange[0];
			g_physicalDeviceProperties.limits.lineWidthRange[1] = deviceProperties.properties.limits.lineWidthRange[1];
			g_physicalDeviceProperties.limits.pointSizeGranularity = deviceProperties.properties.limits.pointSizeGranularity;
			g_physicalDeviceProperties.limits.lineWidthGranularity = deviceProperties.properties.limits.lineWidthGranularity;
			g_physicalDeviceProperties.limits.strictLines = deviceProperties.properties.limits.strictLines;
			g_physicalDeviceProperties.limits.standardSampleLocations = deviceProperties.properties.limits.standardSampleLocations;
			g_physicalDeviceProperties.limits.optimalBufferCopyOffsetAlignment = deviceProperties.properties.limits.optimalBufferCopyOffsetAlignment;
			g_physicalDeviceProperties.limits.optimalBufferCopyRowPitchAlignment = deviceProperties.properties.limits.optimalBufferCopyRowPitchAlignment;
			g_physicalDeviceProperties.limits.nonCoherentAtomSize = deviceProperties.properties.limits.nonCoherentAtomSize;
		}

		// VK_EXT_descriptor_buffer
		{
			g_physicalDeviceProperties.descriptorBufferProperties.enabled = IsExtensionAvailable(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);
			g_physicalDeviceProperties.descriptorBufferProperties.combinedImageSamplerDescriptorSingleArray = descriptorBufferProperties.combinedImageSamplerDescriptorSingleArray;
			g_physicalDeviceProperties.descriptorBufferProperties.bufferlessPushDescriptors = descriptorBufferProperties.bufferlessPushDescriptors;
			g_physicalDeviceProperties.descriptorBufferProperties.allowSamplerImageViewPostSubmitCreation = descriptorBufferProperties.allowSamplerImageViewPostSubmitCreation;
			g_physicalDeviceProperties.descriptorBufferProperties.descriptorBufferOffsetAlignment = descriptorBufferProperties.descriptorBufferOffsetAlignment;
			g_physicalDeviceProperties.descriptorBufferProperties.maxDescriptorBufferBindings = descriptorBufferProperties.maxDescriptorBufferBindings;
			g_physicalDeviceProperties.descriptorBufferProperties.maxResourceDescriptorBufferBindings = descriptorBufferProperties.maxResourceDescriptorBufferBindings;
			g_physicalDeviceProperties.descriptorBufferProperties.maxSamplerDescriptorBufferBindings = descriptorBufferProperties.maxSamplerDescriptorBufferBindings;
			g_physicalDeviceProperties.descriptorBufferProperties.maxEmbeddedImmutableSamplerBindings = descriptorBufferProperties.maxEmbeddedImmutableSamplerBindings;
			g_physicalDeviceProperties.descriptorBufferProperties.maxEmbeddedImmutableSamplers = descriptorBufferProperties.maxEmbeddedImmutableSamplers;
			g_physicalDeviceProperties.descriptorBufferProperties.bufferCaptureReplayDescriptorDataSize = descriptorBufferProperties.bufferCaptureReplayDescriptorDataSize;
			g_physicalDeviceProperties.descriptorBufferProperties.imageCaptureReplayDescriptorDataSize = descriptorBufferProperties.imageCaptureReplayDescriptorDataSize;
			g_physicalDeviceProperties.descriptorBufferProperties.imageViewCaptureReplayDescriptorDataSize = descriptorBufferProperties.imageViewCaptureReplayDescriptorDataSize;
			g_physicalDeviceProperties.descriptorBufferProperties.samplerCaptureReplayDescriptorDataSize = descriptorBufferProperties.samplerCaptureReplayDescriptorDataSize;
			g_physicalDeviceProperties.descriptorBufferProperties.accelerationStructureCaptureReplayDescriptorDataSize = descriptorBufferProperties.accelerationStructureCaptureReplayDescriptorDataSize;
			g_physicalDeviceProperties.descriptorBufferProperties.samplerDescriptorSize = descriptorBufferProperties.samplerDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.combinedImageSamplerDescriptorSize = descriptorBufferProperties.combinedImageSamplerDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.sampledImageDescriptorSize = descriptorBufferProperties.sampledImageDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.storageImageDescriptorSize = descriptorBufferProperties.storageImageDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.uniformTexelBufferDescriptorSize = descriptorBufferProperties.uniformTexelBufferDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.robustUniformTexelBufferDescriptorSize = descriptorBufferProperties.robustUniformTexelBufferDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.storageTexelBufferDescriptorSize = descriptorBufferProperties.storageTexelBufferDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.robustStorageTexelBufferDescriptorSize = descriptorBufferProperties.robustStorageTexelBufferDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.uniformBufferDescriptorSize = descriptorBufferProperties.uniformBufferDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.robustUniformBufferDescriptorSize = descriptorBufferProperties.robustUniformBufferDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.storageBufferDescriptorSize = descriptorBufferProperties.storageBufferDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.robustStorageBufferDescriptorSize = descriptorBufferProperties.robustStorageBufferDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.inputAttachmentDescriptorSize = descriptorBufferProperties.inputAttachmentDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.accelerationStructureDescriptorSize = descriptorBufferProperties.accelerationStructureDescriptorSize;
			g_physicalDeviceProperties.descriptorBufferProperties.maxSamplerDescriptorBufferRange = descriptorBufferProperties.maxSamplerDescriptorBufferRange;
			g_physicalDeviceProperties.descriptorBufferProperties.maxResourceDescriptorBufferRange = descriptorBufferProperties.maxResourceDescriptorBufferRange;
			g_physicalDeviceProperties.descriptorBufferProperties.samplerDescriptorBufferAddressSpaceSize = descriptorBufferProperties.samplerDescriptorBufferAddressSpaceSize;
			g_physicalDeviceProperties.descriptorBufferProperties.resourceDescriptorBufferAddressSpaceSize = descriptorBufferProperties.resourceDescriptorBufferAddressSpaceSize;
			g_physicalDeviceProperties.descriptorBufferProperties.descriptorBufferAddressSpaceSize = descriptorBufferProperties.descriptorBufferAddressSpaceSize;
		}

		// VK_EXT_mesh_shader
		{
			g_physicalDeviceProperties.meshShaderProperties.enabled = IsExtensionAvailable(VK_EXT_MESH_SHADER_EXTENSION_NAME);
			g_physicalDeviceProperties.meshShaderProperties.maxTaskWorkGroupTotalCount = meshShaderProperties.maxTaskWorkGroupTotalCount;
			g_physicalDeviceProperties.meshShaderProperties.maxTaskWorkGroupCount[0] = meshShaderProperties.maxTaskWorkGroupCount[0];
			g_physicalDeviceProperties.meshShaderProperties.maxTaskWorkGroupCount[1] = meshShaderProperties.maxTaskWorkGroupCount[1];
			g_physicalDeviceProperties.meshShaderProperties.maxTaskWorkGroupCount[2] = meshShaderProperties.maxTaskWorkGroupCount[2];
			g_physicalDeviceProperties.meshShaderProperties.maxTaskWorkGroupInvocations = meshShaderProperties.maxTaskWorkGroupInvocations;
			g_physicalDeviceProperties.meshShaderProperties.maxTaskWorkGroupSize[0] = meshShaderProperties.maxTaskWorkGroupSize[0];
			g_physicalDeviceProperties.meshShaderProperties.maxTaskWorkGroupSize[1] = meshShaderProperties.maxTaskWorkGroupSize[1];
			g_physicalDeviceProperties.meshShaderProperties.maxTaskWorkGroupSize[2] = meshShaderProperties.maxTaskWorkGroupSize[2];
			g_physicalDeviceProperties.meshShaderProperties.maxTaskPayloadSize = meshShaderProperties.maxTaskPayloadSize;
			g_physicalDeviceProperties.meshShaderProperties.maxTaskSharedMemorySize = meshShaderProperties.maxTaskSharedMemorySize;
			g_physicalDeviceProperties.meshShaderProperties.maxTaskPayloadAndSharedMemorySize = meshShaderProperties.maxTaskPayloadAndSharedMemorySize;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshWorkGroupTotalCount = meshShaderProperties.maxMeshWorkGroupTotalCount;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshWorkGroupCount[0] = meshShaderProperties.maxMeshWorkGroupCount[0];
			g_physicalDeviceProperties.meshShaderProperties.maxMeshWorkGroupCount[1] = meshShaderProperties.maxMeshWorkGroupCount[1];
			g_physicalDeviceProperties.meshShaderProperties.maxMeshWorkGroupCount[2] = meshShaderProperties.maxMeshWorkGroupCount[2];
			g_physicalDeviceProperties.meshShaderProperties.maxMeshWorkGroupInvocations = meshShaderProperties.maxMeshWorkGroupInvocations;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshWorkGroupSize[0] = meshShaderProperties.maxMeshWorkGroupSize[0];
			g_physicalDeviceProperties.meshShaderProperties.maxMeshWorkGroupSize[1] = meshShaderProperties.maxMeshWorkGroupSize[1];
			g_physicalDeviceProperties.meshShaderProperties.maxMeshWorkGroupSize[2] = meshShaderProperties.maxMeshWorkGroupSize[2];
			g_physicalDeviceProperties.meshShaderProperties.maxMeshSharedMemorySize = meshShaderProperties.maxMeshSharedMemorySize;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshPayloadAndSharedMemorySize = meshShaderProperties.maxMeshPayloadAndSharedMemorySize;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshOutputMemorySize = meshShaderProperties.maxMeshOutputMemorySize;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshPayloadAndOutputMemorySize = meshShaderProperties.maxMeshPayloadAndOutputMemorySize;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshOutputComponents = meshShaderProperties.maxMeshOutputComponents;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshOutputVertices = meshShaderProperties.maxMeshOutputVertices;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshOutputPrimitives = meshShaderProperties.maxMeshOutputPrimitives;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshOutputLayers = meshShaderProperties.maxMeshOutputLayers;
			g_physicalDeviceProperties.meshShaderProperties.maxMeshMultiviewViewCount = meshShaderProperties.maxMeshMultiviewViewCount;
			g_physicalDeviceProperties.meshShaderProperties.meshOutputPerVertexGranularity = meshShaderProperties.meshOutputPerVertexGranularity;
			g_physicalDeviceProperties.meshShaderProperties.meshOutputPerPrimitiveGranularity = meshShaderProperties.meshOutputPerPrimitiveGranularity;
			g_physicalDeviceProperties.meshShaderProperties.maxPreferredTaskWorkGroupInvocations = meshShaderProperties.maxPreferredTaskWorkGroupInvocations;
			g_physicalDeviceProperties.meshShaderProperties.maxPreferredMeshWorkGroupInvocations = meshShaderProperties.maxPreferredMeshWorkGroupInvocations;
			g_physicalDeviceProperties.meshShaderProperties.prefersLocalInvocationVertexOutput = meshShaderProperties.prefersLocalInvocationVertexOutput;
			g_physicalDeviceProperties.meshShaderProperties.prefersLocalInvocationPrimitiveOutput = meshShaderProperties.prefersLocalInvocationPrimitiveOutput;
			g_physicalDeviceProperties.meshShaderProperties.prefersCompactVertexOutput = meshShaderProperties.prefersCompactVertexOutput;
			g_physicalDeviceProperties.meshShaderProperties.prefersCompactPrimitiveOutput = meshShaderProperties.prefersCompactPrimitiveOutput;
		}

		// VK_KHR_ray_tracing_pipeline
		{
			g_physicalDeviceProperties.rayTracingPipelineProperties.shaderGroupHandleSize  = rayTracingPipelineProperties.shaderGroupHandleSize;
			g_physicalDeviceProperties.rayTracingPipelineProperties.maxRayRecursionDepth = rayTracingPipelineProperties.maxRayRecursionDepth;
			g_physicalDeviceProperties.rayTracingPipelineProperties.maxShaderGroupStride = rayTracingPipelineProperties.maxShaderGroupStride;
			g_physicalDeviceProperties.rayTracingPipelineProperties.shaderGroupBaseAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;
			g_physicalDeviceProperties.rayTracingPipelineProperties.shaderGroupHandleCaptureReplaySize = rayTracingPipelineProperties.shaderGroupHandleCaptureReplaySize;
			g_physicalDeviceProperties.rayTracingPipelineProperties.maxRayDispatchInvocationCount = rayTracingPipelineProperties.maxRayDispatchInvocationCount;
			g_physicalDeviceProperties.rayTracingPipelineProperties.shaderGroupHandleAlignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
			g_physicalDeviceProperties.rayTracingPipelineProperties.maxRayHitAttributeSize = rayTracingPipelineProperties.maxRayHitAttributeSize;
		}

		// VK_KHR_acceleration_structure
		{
			g_physicalDeviceProperties.accelerationStructureProperties.maxGeometryCount = accelerationStructureProperties.maxGeometryCount;
			g_physicalDeviceProperties.accelerationStructureProperties.maxInstanceCount = accelerationStructureProperties.maxInstanceCount;
			g_physicalDeviceProperties.accelerationStructureProperties.maxPrimitiveCount = accelerationStructureProperties.maxPrimitiveCount;
			g_physicalDeviceProperties.accelerationStructureProperties.maxPerStageDescriptorAccelerationStructures = accelerationStructureProperties.maxPerStageDescriptorAccelerationStructures;
			g_physicalDeviceProperties.accelerationStructureProperties.maxPerStageDescriptorUpdateAfterBindAccelerationStructures = accelerationStructureProperties.maxPerStageDescriptorUpdateAfterBindAccelerationStructures;
			g_physicalDeviceProperties.accelerationStructureProperties.maxDescriptorSetAccelerationStructures = accelerationStructureProperties.maxDescriptorSetAccelerationStructures;
			g_physicalDeviceProperties.accelerationStructureProperties.maxDescriptorSetUpdateAfterBindAccelerationStructures = accelerationStructureProperties.maxDescriptorSetUpdateAfterBindAccelerationStructures;
			g_physicalDeviceProperties.accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment = accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;
		}

		FetchMemoryProperties();
	}

	void VulkanPhysicalGraphicsDevice::FetchAvailableExtensions()
	{
		uint32_t extCount = 0;
		VT_VK_CHECK(vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extCount, nullptr));
		
		m_availableExtensions.resize(extCount);
		VT_VK_CHECK(vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extCount, reinterpret_cast<VkExtensionProperties*>(m_availableExtensions.data())));
	}
}
