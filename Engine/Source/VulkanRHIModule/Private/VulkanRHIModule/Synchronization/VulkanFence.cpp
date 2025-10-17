#include "vkpch.h"

#include "VulkanRHIModule/Synchronization/VulkanFence.h"

#include <RHIModule/Graphics/GraphicsContext.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanFence::VulkanFence()
	{

	}

	VulkanFence::~VulkanFence()
	{

	}

	void VulkanFence::WaitUntilSignaled() const
	{
		if (m_referencedSemaphore)
		{
			VkSemaphoreWaitInfo waitInfo{};
			waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
			waitInfo.semaphoreCount = 1;
			waitInfo.pSemaphores = &m_referencedSemaphore;
			waitInfo.pValues = &m_referencedValue;

			auto device = GraphicsContext::GetDevice();
			vkWaitSemaphores(device->GetHandle<VkDevice>(), &waitInfo, UINT64_MAX);
		}
	}

	void* VulkanFence::GetHandleImpl() const
	{
		return nullptr;
	}

	bool VulkanFence::IsSignaled() const
	{
		if (m_referencedSemaphore)
		{
			uint64_t value;
			auto device = GraphicsContext::GetDevice();
			vkGetSemaphoreCounterValue(device->GetHandle<VkDevice>(), m_referencedSemaphore, &value);

			return value >= m_referencedValue;
		}

		// If no fence is referenced, we treat it as signaled.
		return true;
	}

	void VulkanFence::Reset()
	{
		m_referencedSemaphore = nullptr;
	}
}
