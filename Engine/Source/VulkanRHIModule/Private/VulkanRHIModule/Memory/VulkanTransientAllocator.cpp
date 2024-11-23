#include "vkpch.h"
#include "VulkanRHIModule/Memory/VulkanTransientAllocator.h"

#include "VulkanRHIModule/Memory/VulkanAllocation.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/PhysicalGraphicsDevice.h>
#include <RHIModule/Graphics/GraphicsDevice.h>

#include <RHIModule/Memory/MemoryUtility.h>
#include <RHIModule/Core/Profiling.h>
#include <RHIModule/Utility/HashUtility.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanTransientAllocator::VulkanTransientAllocator()
	{
		CreateDefaultHeaps();
	}

	VulkanTransientAllocator::~VulkanTransientAllocator()
	{
		for (const auto& imageAlloc : m_allocationCache.GetImageAllocations())
		{
			DestroyImageInternal(imageAlloc.allocation);
		}

		for (const auto& bufferAlloc : m_allocationCache.GetBufferAllocations())
		{
			DestroyBufferInternal(bufferAlloc.allocation);
		}

		m_bufferHeaps.clear();
		m_imageHeaps.clear();
	}

	Handle<Allocation> VulkanTransientAllocator::CreateBuffer(const uint64_t size, BufferUsage usage, MemoryUsage memoryUsage, const std::string& name)
	{
		VT_PROFILE_FUNCTION();

		const size_t hash = Utility::GetHashFromBufferSpec(size, usage, memoryUsage);
		if (auto buffer = m_allocationCache.TryGetBufferAllocationFromHash(hash))
		{
			return buffer;
		}

		TransientBufferCreateInfo info{};
		info.size = size;
		info.usage = usage;
		info.memoryUsage = memoryUsage;
		info.hash = hash;

		TransientHeapFlags heapFlags = TransientHeapFlags::AllowBuffers;
		if ((memoryUsage & MemoryUsage::CPUToGPU) != MemoryUsage::None)
		{
			heapFlags |= TransientHeapFlags::AllowMappable;
		}

		Handle<Allocation> result;

		for (const auto& heap : m_bufferHeaps)
		{
			if (heap->IsAllocationSupported(size, heapFlags))
			{
				result = heap->CreateBuffer(info, name);
				break;
			}
		}

		// If we were not able to allocate in the heap, we will create a new one
		if (!result)
		{
			auto heap = CreateNewBufferHeap(heapFlags);
			if (heap->IsAllocationSupported(size, heapFlags))
			{
				result = heap->CreateBuffer(info, name);
			}
		}

		if (!result)
		{
			VT_LOGC(Error, LogVulkanRHI, "Unable to create buffer of size {0}!", size);
		}

		return result;
	}

	Handle<Allocation> VulkanTransientAllocator::CreateImage(const ImageSpecification& imageSpecification, MemoryUsage memoryUsage)
	{
		VT_PROFILE_FUNCTION();

		const size_t hash = Utility::GetHashFromImageSpec(imageSpecification, memoryUsage);
		if (auto image = m_allocationCache.TryGetImageAllocationFromHash(hash))
		{
			return image;
		}

		MemoryRequirement memoryRequirement = Utility::GetImageRequirement(Utility::GetVkImageCreateInfo(imageSpecification));

		TransientImageCreateInfo info{};
		info.imageSpecification = imageSpecification;
		info.size = Utility::Align(memoryRequirement.size, memoryRequirement.alignment);
		info.hash = hash;

		Handle<Allocation> result;

		for (const auto& heap : m_imageHeaps)
		{
			if (heap->IsAllocationSupported(info.size, TransientHeapFlags::AllowTextures))
			{
				result = heap->CreateImage(info, imageSpecification.debugName);
				break;
			}
		}

		// If we were not able to allocate in the heap, we will create a new one
		if (!result)
		{
			auto heap = CreateNewImageHeap();
			if (heap->IsAllocationSupported(info.size, TransientHeapFlags::AllowTextures))
			{
				result = heap->CreateImage(info, imageSpecification.debugName);
			}
		}

		if (!result)
		{
			VT_LOGC(Error, LogVulkanRHI, "Unable to create image of size {0}!", memoryRequirement.size);
		}

		return result;
	}

	void VulkanTransientAllocator::DestroyBuffer(Handle<Allocation> allocation)
	{
		m_allocationCache.QueueBufferAllocationForRemoval(allocation);
	}

	void VulkanTransientAllocator::DestroyImage(Handle<Allocation> allocation)
	{
		m_allocationCache.QueueImageAllocationForRemoval(allocation);
	}

	Vector<Handle<Allocation>> VulkanTransientAllocator::GetActiveBufferAllocations() const
	{
		return Vector<Handle<Allocation>>();
	}

	Vector<Handle<Allocation>> VulkanTransientAllocator::GetActiveImageAllocations() const
	{
		return Vector<Handle<Allocation>>();
	}

	void* VulkanTransientAllocator::GetHandleImpl() const
	{
		return nullptr;
	}

	void VulkanTransientAllocator::CreateDefaultHeaps()
	{
		// Buffer heap
		{
			TransientHeapCreateInfo info{};
			info.pageSize = HEAP_PAGE_SIZE;
			info.flags = TransientHeapFlags::AllowBuffers;
			m_bufferHeaps.push_back(TransientHeap::Create(info));
		}

		// Image heap
		{
			TransientHeapCreateInfo info{};
			info.pageSize = HEAP_PAGE_SIZE;
			info.flags = TransientHeapFlags::AllowTextures | TransientHeapFlags::AllowRenderTargets;
			m_imageHeaps.push_back(TransientHeap::Create(info));
		}
	}

	void VulkanTransientAllocator::DestroyBufferInternal(Handle<Allocation> allocation)
	{
		VT_PROFILE_FUNCTION();

		RefPtr<TransientHeap> parentHeap;

		for (const auto& heap : m_bufferHeaps)
		{
			if (heap->GetHeapID() == allocation->GetHeapID())
			{
				parentHeap = heap;
				break;
			}
		}

		if (parentHeap)
		{
			parentHeap->ForfeitBuffer(allocation);
		}
		else
		{
			VT_LOGC(Error, LogVulkanRHI, "Unable to destroy buffer with heap ID {0}!", static_cast<uint64_t>(allocation->GetHeapID()));
			DestroyOrphanBuffer(allocation);
		}
	}

	void VulkanTransientAllocator::DestroyImageInternal(Handle<Allocation> allocation)
	{
		VT_PROFILE_FUNCTION();

		RefPtr<TransientHeap> parentHeap;

		for (const auto& heap : m_imageHeaps)
		{
			if (heap->GetHeapID() == allocation->GetHeapID())
			{
				parentHeap = heap;
				break;
			}
		}

		if (parentHeap)
		{
			parentHeap->ForfeitImage(allocation);
		}
		else
		{
			VT_LOGC(Error, LogVulkanRHI, "Unable to destroy image with heap ID {0}!", static_cast<uint64_t>(allocation->GetHeapID()));
			DestroyOrphanImage(allocation);
		}
	}

	void VulkanTransientAllocator::DestroyOrphanBuffer(Handle<Allocation> allocation)
	{
		auto device = GraphicsContext::GetDevice();
		vkDestroyBuffer(device->GetHandle<VkDevice>(), allocation->GetResourceHandle<VkBuffer>(), nullptr);
	}

	void VulkanTransientAllocator::DestroyOrphanImage(Handle<Allocation> allocation)
	{
		auto device = GraphicsContext::GetDevice();
		vkDestroyImage(device->GetHandle<VkDevice>(), allocation->GetResourceHandle<VkImage>(), nullptr);
	}

	RefPtr<TransientHeap> VulkanTransientAllocator::CreateNewImageHeap()
	{
		TransientHeapCreateInfo info{};
		info.pageSize = HEAP_PAGE_SIZE;
		info.flags = TransientHeapFlags::AllowTextures | TransientHeapFlags::AllowRenderTargets;
		
		RefPtr<TransientHeap>& heap = m_imageHeaps.emplace_back();
		heap = TransientHeap::Create(info);

		return heap;
	}

	RefPtr<TransientHeap> VulkanTransientAllocator::CreateNewBufferHeap(TransientHeapFlags heapFlags)
	{
		TransientHeapCreateInfo info{};
		info.pageSize = HEAP_PAGE_SIZE;
		info.flags = heapFlags;

		RefPtr<TransientHeap>& heap = m_bufferHeaps.emplace_back();
		heap = TransientHeap::Create(info);

		return heap;
	}

	void VulkanTransientAllocator::Update()
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
}
