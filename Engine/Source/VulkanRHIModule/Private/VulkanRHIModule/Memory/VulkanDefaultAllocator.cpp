#include "vkpch.h"
#include "VulkanRHIModule/Memory/VulkanDefaultAllocator.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Memory/VulkanAllocation.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>

#include <RHIModule/Core/Profiling.h>
#include <RHIModule/Memory/MemoryUtility.h>
#include <RHIModule/Utility/HashUtility.h>

#include <CoreUtilities/Allocators/Handle.h>

#include <vma/VulkanMemoryAllocator.h>

namespace Volt::RHI
{
	VulkanDefaultAllocator::VulkanDefaultAllocator()
	{
		VmaAllocatorCreateInfo info{};
		info.vulkanApiVersion = VK_API_VERSION_1_3;
		info.physicalDevice = GraphicsContext::GetPhysicalDevice()->GetHandle<VkPhysicalDevice>();
		info.device = GraphicsContext::GetDevice()->GetHandle<VkDevice>();
		info.instance = GraphicsContext::Get().GetHandle<VkInstance>();
		info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		if (GraphicsContext::GetPhysicalDevice()->AsRef<VulkanPhysicalGraphicsDevice>().AreDescriptorBuffersEnabled())
		{
			info.flags |= VMA_ALLOCATOR_CREATE_EXT_DESCRIPTOR_BUFFER_BIT;
		}

		VT_VK_CHECK(vmaCreateAllocator(&info, &m_allocator));
	}

	VulkanDefaultAllocator::~VulkanDefaultAllocator()
	{
		auto activeImageAllocations = GetActiveImageAllocations();
		for (const auto& alloc : activeImageAllocations)
		{
			DestroyImageInternal(alloc);
		}

		auto activeBufferAllocations = GetActiveBufferAllocations();
		for (const auto& alloc : activeBufferAllocations)
		{
			DestroyBufferInternal(alloc);
		}

		vmaDestroyAllocator(m_allocator);
	}

	Handle<Allocation> VulkanDefaultAllocator::CreateBuffer(const size_t size, BufferUsage usage, MemoryUsage memoryUsage, const std::string& name)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(size > 0);

		const size_t hash = Utility::GetHashFromBufferSpec(size, usage, memoryUsage);

		{
			std::scoped_lock lock{ m_bufferAllocationMutex };
			if (auto buffer = m_allocationCache.TryGetBufferAllocationFromHash(hash))
			{
				return buffer;
			}
		}

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;
		bufferInfo.pQueueFamilyIndices = nullptr;
		bufferInfo.queueFamilyIndexCount = 0;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; 
		bufferInfo.size = size;
		bufferInfo.usage = Utility::GetVkBufferUsageFlags(usage);

		if (GraphicsContext::GetPhysicalDevice()->AsRef<VulkanPhysicalGraphicsDevice>().AreDescriptorBuffersEnabled())
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		}

		VmaMemoryUsage usageFlags = VMA_MEMORY_USAGE_AUTO;
		VmaAllocationCreateFlags createFlags = 0;

		if ((memoryUsage & MemoryUsage::CPU) != MemoryUsage::None)
		{
			usageFlags = VMA_MEMORY_USAGE_CPU_ONLY;
			createFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else if ((memoryUsage & MemoryUsage::CPUToGPU) != MemoryUsage::None)
		{
			createFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else if ((memoryUsage & MemoryUsage::GPUToCPU) != MemoryUsage::None)
		{
			createFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}

		if ((memoryUsage & MemoryUsage::Dedicated) != MemoryUsage::None)
		{
			createFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}

		VmaAllocationCreateInfo allocCreateInfo{};
		allocCreateInfo.usage = usageFlags;
		allocCreateInfo.flags = createFlags;
		allocCreateInfo.memoryTypeBits = 0;
		allocCreateInfo.preferredFlags = 0;
		allocCreateInfo.priority = 0.f;
		allocCreateInfo.pool = nullptr;
		allocCreateInfo.pUserData = nullptr;

		VmaAllocationInfo allocInfo{};

		Handle<VulkanBufferAllocation> allocation = m_bufferAllocationArena.Allocate(hash, name);
		VT_VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &allocCreateInfo, &allocation->m_resource, &allocation->m_allocation, &allocInfo));

		allocation->m_size = size;

		return allocation;
	}

	Handle<Allocation> VulkanDefaultAllocator::CreateImage(const ImageSpecification& imageSpecification, MemoryUsage memoryUsage)
	{
		VT_PROFILE_FUNCTION();

		const size_t hash = Utility::GetHashFromImageSpec(imageSpecification, memoryUsage);

		{
			std::scoped_lock lock{ m_imageAllocationMutex };

			if (auto image = m_allocationCache.TryGetImageAllocationFromHash(hash))
			{
				return image;
			}
		}

		const VkImageCreateInfo imageInfo = Utility::GetVkImageCreateInfo(imageSpecification);

		VmaMemoryUsage usageFlags = VMA_MEMORY_USAGE_AUTO;
		VmaAllocationCreateFlags createFlags = 0;

		if ((memoryUsage & MemoryUsage::CPU) != MemoryUsage::None)
		{
			usageFlags = VMA_MEMORY_USAGE_CPU_ONLY;
			createFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else if ((memoryUsage & MemoryUsage::CPUToGPU) != MemoryUsage::None)
		{
			createFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else if ((memoryUsage & MemoryUsage::GPUToCPU) != MemoryUsage::None)
		{
			createFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}

		if ((memoryUsage & MemoryUsage::Dedicated) != MemoryUsage::None)
		{
			createFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		}

		VmaAllocationCreateInfo allocCreateInfo{};
		allocCreateInfo.usage = usageFlags;
		allocCreateInfo.flags = createFlags;
		allocCreateInfo.priority = 1.f;

		VmaAllocationInfo allocInfo{};

		Handle<VulkanImageAllocation> allocation = m_imageAllocationArena.Allocate(hash, imageSpecification.debugName);
		VT_VK_CHECK(vmaCreateImage(m_allocator, &imageInfo, &allocCreateInfo, &allocation->m_resource, &allocation->m_allocation, &allocInfo));

		if (!imageSpecification.debugName.empty())
		{
			vmaSetAllocationName(m_allocator, allocation->m_allocation, imageSpecification.debugName.c_str());
		}

		// Get Size
		{
			VmaAllocationInfo info{};
			vmaGetAllocationInfo(m_allocator, allocation->m_allocation, &info);
			allocation->m_size = info.size;
		}

		return allocation;
	}

	void VulkanDefaultAllocator::DestroyBuffer(Handle<Allocation> allocation)
	{
		m_allocationCache.QueueBufferAllocationForRemoval(allocation);
	}

	void VulkanDefaultAllocator::DestroyImage(Handle<Allocation> allocation)
	{
		m_allocationCache.QueueImageAllocationForRemoval(allocation);
	}

	Vector<Handle<Allocation>> VulkanDefaultAllocator::GetActiveBufferAllocations() const
	{
		auto activeAllocations = m_bufferAllocationArena.GetActiveAllocations();

		Vector<Handle<Allocation>> result;
		result.reserve(activeAllocations.size());

		for (const auto& alloc : activeAllocations)
		{
			result.emplace_back(Handle<Allocation>(alloc));
		}

		return result;
	}

	Vector<Handle<Allocation>> VulkanDefaultAllocator::GetActiveImageAllocations() const
	{
		auto activeAllocations = m_imageAllocationArena.GetActiveAllocations();

		Vector<Handle<Allocation>> result;
		result.reserve(activeAllocations.size());

		for (const auto& alloc : activeAllocations)
		{
			result.emplace_back(Handle<Allocation>(alloc));
		}

		return result;
	}

	void VulkanDefaultAllocator::Update()
	{
		const auto allocationsToRemove = m_allocationCache.UpdateAndGetAllocationsToDestroy();
		
		for (const auto& alloc : allocationsToRemove.bufferAllocations)
		{
			DestroyBufferInternal(alloc);
		}

		for (const auto& alloc : allocationsToRemove.imageAllocations)
		{
			DestroyImageInternal(alloc);
		}
	}

	void VulkanDefaultAllocator::DestroyBufferInternal(Handle<Allocation> allocation)
	{
		VT_PROFILE_FUNCTION();

		auto bufferAlloc = allocation.As<VulkanBufferAllocation>();
		vmaDestroyBuffer(m_allocator, bufferAlloc->m_resource, bufferAlloc->m_allocation);

		m_bufferAllocationArena.Free(bufferAlloc.GetRaw());
	}

	void VulkanDefaultAllocator::DestroyImageInternal(Handle<Allocation> allocation)
	{
		VT_PROFILE_FUNCTION();

		auto imageAlloc = allocation.As<VulkanImageAllocation>();
		vmaDestroyImage(m_allocator, imageAlloc->m_resource, imageAlloc->m_allocation);
		
		m_imageAllocationArena.Free(imageAlloc.GetRaw());
	}

	void* VulkanDefaultAllocator::GetHandleImpl() const
	{
		return m_allocator;
	}
}
