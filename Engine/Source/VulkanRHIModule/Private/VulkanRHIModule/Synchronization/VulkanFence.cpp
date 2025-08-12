#include "vkpch.h"
#include "VulkanRHIModule/Synchronization/VulkanFence.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>

#include <RHIModule/RHIModule.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanFence::VulkanFence(const FenceCreateInfo& createInfo)
	{
		VkFenceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = createInfo.createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

		m_isExecuted = createInfo.createSignaled;

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateFence(device->GetHandle<VkDevice>(), &info, VT_VULKAN_ALLOCATOR, &m_fence));
	}

	VulkanFence::~VulkanFence()
	{
		RHIModule::GetInstance().DestroyResource([fence = m_fence]()
		{
			auto device = GraphicsContext::GetDevice();
			vkDestroyFence(device->GetHandle<VkDevice>(), fence, VT_VULKAN_ALLOCATOR);
		});
	}

	void VulkanFence::Reset() const
	{
		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkResetFences(device->GetHandle<VkDevice>(), 1, &m_fence));
	
		m_isExecuted = false;
	}

	FenceStatus VulkanFence::GetStatus() const
	{
		auto device = GraphicsContext::GetDevice();
		VkResult result = vkGetFenceStatus(device->GetHandle<VkDevice>(), m_fence);

		if (result == VK_SUCCESS)
		{
			return FenceStatus::Signaled;
		}
		else if (result == VK_NOT_READY)
		{
			return FenceStatus::Unsignaled;
		}

		return FenceStatus::Error;
	}

	void VulkanFence::WaitUntilSignaled() const
	{
		// Make sure we wait for the fence to be used in an execution call before 
		// we enter the wait for fences call.
		m_isExecuted.wait(false, std::memory_order::relaxed);

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkWaitForFences(device->GetHandle<VkDevice>(), 1, &m_fence, VK_TRUE, UINT64_MAX));
	}

	void* VulkanFence::GetHandleImpl() const
	{
		return m_fence;
	}

	void VulkanFence::MarkAsExecuted()
	{
		m_isExecuted = true;
		m_isExecuted.notify_all();
	}

	bool VulkanFence::HasBeenExecuted() const
	{
		return m_isExecuted.load(std::memory_order::relaxed);
	}
}
