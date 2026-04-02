#include "vkpch.h"

#include "VulkanRHIModule/Synchronization/VulkanSemaphore.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIModule.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanSemaphore::VulkanSemaphore()
	{
		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateSemaphore(device->GetHandle<VkDevice>(), &createInfo, VT_VULKAN_ALLOCATOR, &m_semaphore));
	}

	VulkanSemaphore::~VulkanSemaphore()
	{
		RHIModule::GetInstance().DestroyResource([semaphore = m_semaphore]() 
		{
			auto device = GraphicsContext::GetDevice();
			vkDestroySemaphore(device->GetHandle<VkDevice>(), semaphore, VT_VULKAN_ALLOCATOR);
		});
	}

	void* VulkanSemaphore::GetHandleImpl() const
	{
		return m_semaphore;
	}
}
