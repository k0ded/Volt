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
		if (!m_hasBeenSubmitted.load(std::memory_order::relaxed))
		{
			return;
		}

		// Wait for the semaphore value to be set.
		m_referencedValue.wait(0, std::memory_order::relaxed);

		if (m_referencedSemaphore)
		{
			uint64_t tempValue = m_referencedValue.load(std::memory_order::relaxed);

			VkSemaphoreWaitInfo waitInfo{};
			waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
			waitInfo.semaphoreCount = 1;
			waitInfo.pSemaphores = &m_referencedSemaphore;
			waitInfo.pValues = &tempValue;

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

	void VulkanFence::AssignSemaphore(VkSemaphore_T* semaphore, uint64_t value)
	{
		m_referencedSemaphore = semaphore;
		m_referencedValue.store(value, std::memory_order::relaxed);
		m_referencedValue.notify_all();
	}
}
