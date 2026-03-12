#pragma once

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	extern VkAllocationCallbacks g_vulkanAllocationCallbacks;

	class VulkanCPUAllocator
	{
	public:
		VulkanCPUAllocator();

	private:
		static void* Alloc(void* userData, size_t size, size_t alignment, VkSystemAllocationScope allocScope);
		static void Free(void* userData, void* memory);
		static void* Realloc(void* userData, void* original, size_t size, size_t alignment, VkSystemAllocationScope allocScope);

		static void InternalAllocationNotification(void* userData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocScope);
		static void InternalFreeNotification(void* userData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocScope);
	};

	VT_NODISCARD VT_INLINE const VkAllocationCallbacks* GetAllocationCallbacks()
	{
		return &g_vulkanAllocationCallbacks;
	}
}

//#define VT_VULKAN_ALLOCATOR Volt::RHI::GetAllocationCallbacks()
#define VT_VULKAN_ALLOCATOR nullptr
