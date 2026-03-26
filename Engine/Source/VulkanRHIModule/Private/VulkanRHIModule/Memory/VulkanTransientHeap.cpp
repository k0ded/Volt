#include "vkpch.h"

#include "VulkanRHIModule/Memory/VulkanTransientHeap.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"
#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/VulkanResourceCast.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>

#include <CoreUtilities/MemoryUtility.h>

namespace Volt::RHI
{
	VulkanTransientHeap::VulkanTransientHeap(const TransientHeapCreateInfo& info)
		: m_createInfo(info)
	{
		if ((info.flags & TransientHeapFlags::AllowBuffers) != TransientHeapFlags::None)
		{
			InitializeAsBufferHeap();
		}
		else if ((info.flags & TransientHeapFlags::AllowTextures) != TransientHeapFlags::None || 
			(info.flags & TransientHeapFlags::AllowRenderTargets) != TransientHeapFlags::None)
		{
			InitializeAsImageHeap();
		}
	}

	VulkanTransientHeap::~VulkanTransientHeap()
	{
		auto device = GraphicsContext::GetDevice();

		for (MemoryPage& page : m_memoryPages)
		{
			vkFreeMemory(device->GetHandle<VkDevice>(), page.memoryHandle, VT_VULKAN_ALLOCATOR);
		}
	}

	void* VulkanTransientHeap::GetHandleImpl() const
	{
		return nullptr;
	}

	void VulkanTransientHeap::InitializeAsBufferHeap()
	{
		constexpr VkBufferUsageFlags USAGE_FLAGS = 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | 
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
			VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
			VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;

		VkBufferCreateInfo vkBufferInfo{};
		vkBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vkBufferInfo.pNext = nullptr;
		vkBufferInfo.flags = 0;
		vkBufferInfo.pQueueFamilyIndices = nullptr;
		vkBufferInfo.queueFamilyIndexCount = 0;
		vkBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vkBufferInfo.size = m_createInfo.pageSize;
		vkBufferInfo.usage = USAGE_FLAGS;

		m_memoryRequirements = Utility::GetBufferMemoryRequirement(vkBufferInfo);

		// Initialize with one page.
		AllocateNewPage(0);
	}

	void VulkanTransientHeap::InitializeAsImageHeap()
	{
		constexpr VkImageUsageFlags USAGE_FLAGS = 
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT | 
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkImageCreateInfo vkImageInfo{};
		vkImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		vkImageInfo.pNext = nullptr;
		vkImageInfo.flags = 0;
		vkImageInfo.imageType = VK_IMAGE_TYPE_2D;
		vkImageInfo.format = VK_FORMAT_D32_SFLOAT;
		vkImageInfo.extent.width = 1;
		vkImageInfo.extent.height = 1;
		vkImageInfo.extent.depth = 1;
		vkImageInfo.mipLevels = 1;
		vkImageInfo.arrayLayers = 1;
		vkImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		vkImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		vkImageInfo.usage = USAGE_FLAGS;
		vkImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vkImageInfo.queueFamilyIndexCount = 0;
		vkImageInfo.pQueueFamilyIndices = nullptr;
		vkImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		m_memoryRequirements = Utility::GetImageMemoryRequirement(vkImageInfo);

		// Initialize with one page.
		AllocateNewPage(0);
	}

	void VulkanTransientHeap::AllocateNewPage(uint64_t minSize)
	{
		auto device = GraphicsContext::GetDevice();
		auto physicalDevice = ResourceCast(GraphicsContext::GetPhysicalDevice());

		constexpr VkMemoryPropertyFlags MemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		const int32_t memoryTypeIndex = physicalDevice->GetMemoryTypeIndex(m_memoryRequirements.memoryTypeBits, MemoryFlags);
		VT_ENSURE_MSG(memoryTypeIndex != -1, "Memory type is not supported on this device!");

		VkMemoryAllocateFlagsInfo memoryAllocFlags{};
		memoryAllocFlags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocFlags.pNext = nullptr;
		memoryAllocFlags.deviceMask = 0;
		memoryAllocFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

		const uint64_t alignedSize = std::max(::Utility::Align(minSize, m_memoryRequirements.alignment), ::Utility::Align(m_createInfo.pageSize, m_memoryRequirements.alignment));

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = &memoryAllocFlags;
		allocInfo.allocationSize = alignedSize;
		allocInfo.memoryTypeIndex = static_cast<uint32_t>(memoryTypeIndex);

		VkDeviceMemory memoryHandle = nullptr;
		VT_VK_CHECK(vkAllocateMemory(device->GetHandle<VkDevice>(), &allocInfo, VT_VULKAN_ALLOCATOR, &memoryHandle));

		if (RHI::vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
			nameInfo.objectHandle = std::bit_cast<uint64_t>(memoryHandle);
		
			constexpr StringView name = "Transient Heap Page";
			nameInfo.pObjectName = name.data();

			RHI::vkSetDebugUtilsObjectNameEXT(device->GetHandle<VkDevice>(), &nameInfo);
		}

		m_memoryPages.emplace_back(memoryHandle, alignedSize);
	}

	void VulkanTransientHeap::ReservePages(uint32_t numPages)
	{
		if (static_cast<uint32_t>(m_memoryPages.size()) < numPages)
		{
			for (size_t i = m_memoryPages.size(); i < numPages; ++i)
			{
				AllocateNewPage(0);
			}
		}
	}

	VkDeviceMemory_T* VulkanTransientHeap::GetPageMemoryHandle(uint32_t pageIndex)
	{
		return m_memoryPages[pageIndex].memoryHandle;
	}
}
