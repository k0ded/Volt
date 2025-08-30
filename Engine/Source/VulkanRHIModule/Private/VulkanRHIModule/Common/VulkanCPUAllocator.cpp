#include "vkpch.h"
#include "VulkanCPUAllocator.h"

#include <CoreUtilities/Malloc.h>

namespace Volt::RHI
{
	VkAllocationCallbacks g_vulkanAllocationCallbacks;

	constexpr int32_t VK_SYSTEM_ALLOCATION_SCOPE_RANGE_SIZE = 5;

	VulkanCPUAllocator::VulkanCPUAllocator()
	{
		g_vulkanAllocationCallbacks.pUserData = nullptr;
		g_vulkanAllocationCallbacks.pfnAllocation = (PFN_vkAllocationFunction)&VulkanCPUAllocator::Alloc;
		g_vulkanAllocationCallbacks.pfnReallocation = (PFN_vkReallocationFunction)&VulkanCPUAllocator::Realloc;
		g_vulkanAllocationCallbacks.pfnFree = (PFN_vkFreeFunction)&VulkanCPUAllocator::Free;
		g_vulkanAllocationCallbacks.pfnInternalAllocation = (PFN_vkInternalAllocationNotification)&VulkanCPUAllocator::InternalAllocationNotification;
		g_vulkanAllocationCallbacks.pfnInternalFree = (PFN_vkInternalFreeNotification)&VulkanCPUAllocator::InternalFreeNotification;
	}

	void* VulkanCPUAllocator::Alloc(void* userData, size_t size, size_t alignment, VkSystemAllocationScope allocScope)
	{
		VT_ENSURE(allocScope < VK_SYSTEM_ALLOCATION_SCOPE_RANGE_SIZE);
		return Memory::Malloc(size, alignment);
	}
	
	void VulkanCPUAllocator::Free(void* userData, void* memory)
	{
		Memory::Free(memory);
	}
	
	void* VulkanCPUAllocator::Realloc(void* userData, void* original, size_t size, size_t alignment, VkSystemAllocationScope allocScope)
	{
		VT_ENSURE(allocScope < VK_SYSTEM_ALLOCATION_SCOPE_RANGE_SIZE);
		return Memory::Realloc(original, size, alignment);
	}
	
	void VulkanCPUAllocator::InternalAllocationNotification(void* userData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocScope)
	{
		VT_ENSURE(allocScope < VK_SYSTEM_ALLOCATION_SCOPE_RANGE_SIZE);
	}
	
	void VulkanCPUAllocator::InternalFreeNotification(void* userData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocScope)
	{
		VT_ENSURE(allocScope < VK_SYSTEM_ALLOCATION_SCOPE_RANGE_SIZE);
	}
}
