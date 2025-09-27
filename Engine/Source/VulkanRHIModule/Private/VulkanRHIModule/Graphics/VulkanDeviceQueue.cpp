#include "vkpch.h"
#include "VulkanRHIModule/Graphics/VulkanDeviceQueue.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"

#include "VulkanRHIModule/Graphics/VulkanGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"
#include "VulkanRHIModule/Synchronization/VulkanSemaphore.h"
#include "VulkanRHIModule/Synchronization/VulkanFence.h"
#include "VulkanRHIModule/Synchronization/VulkanFence_New.h"

#include <RHIModule/Graphics/GraphicsContext.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Containers/VectorVariants.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanDeviceQueue::VulkanDeviceQueue(const DeviceQueueCreateInfo& createInfo)
	{
		VulkanGraphicsDevice& graphicsDevice = createInfo.graphicsDevice->AsRef<VulkanGraphicsDevice>();

		const auto physicalDevice = graphicsDevice.GetPhysicalDevice();

		const auto queueFamilies = physicalDevice->GetQueueFamilies();

		uint32_t queueFamily = 0;

		switch (createInfo.queueType)
		{
			case QueueType::Compute: queueFamily = queueFamilies.computeFamilyQueueIndex; break;
			case QueueType::Graphics: queueFamily = queueFamilies.graphicsFamilyQueueIndex; break;
			case QueueType::TransferCopy: queueFamily = queueFamilies.transferFamilyQueueIndex; break;

			default: queueFamily = 0;  break;
		}

		vkGetDeviceQueue(graphicsDevice.GetHandle<VkDevice>(), queueFamily, 0, &m_queue);
		CreateQueueSemaphore(graphicsDevice);
	}

	VulkanDeviceQueue::~VulkanDeviceQueue()
	{
		m_queue = nullptr;
	}

	void VulkanDeviceQueue::WaitForQueue()
	{
		VT_PROFILE_FUNCTION();

		std::scoped_lock lock{ m_executeMutex };
		vkQueueWaitIdle(m_queue);
	}

	void VulkanDeviceQueue::Execute(const DeviceQueueExecuteInfo& executeInfo)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(!executeInfo.commandBuffers.empty(), "Empty execution is invalid!");

		InlineVector<VkCommandBufferSubmitInfo, 64> vulkanCommandBuffers;
		vulkanCommandBuffers.reserve(executeInfo.commandBuffers.size());

		InlineVector<VkSemaphoreSubmitInfo, 64> signalSemaphoreInfos{};
		signalSemaphoreInfos.reserve(executeInfo.signalSemaphores.size());

		for (const auto& cmdBuffer : executeInfo.commandBuffers)
		{
			VulkanCommandBuffer& vkCmdBuffer = cmdBuffer->AsRef<VulkanCommandBuffer>();
			
			vkCmdBuffer.m_submissionFence = Fence_New::Create();

			auto& info = vulkanCommandBuffers.emplace_back();
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
			info.pNext = nullptr;
			info.deviceMask = 0;
			info.commandBuffer = vkCmdBuffer.GetHandle<VkCommandBuffer>();
		}

		for (const auto& semaphore : executeInfo.signalSemaphores)
		{
			auto& info = signalSemaphoreInfos.emplace_back();
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			info.pNext = nullptr;
			info.deviceIndex = 0;
			info.stageMask = VK_PIPELINE_STAGE_2_NONE;
			info.semaphore = semaphore->GetHandle<VkSemaphore>();;
			info.value = semaphore->GetValue();
		}

		{
			auto& queueSemaphore = signalSemaphoreInfos.emplace_back();
			queueSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			queueSemaphore.pNext = nullptr;
			queueSemaphore.deviceIndex = 0;
			queueSemaphore.stageMask = VK_PIPELINE_STAGE_2_NONE;
			queueSemaphore.semaphore = m_queueSemaphore;
		}

		VkSubmitInfo2 info{};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		info.commandBufferInfoCount = static_cast<uint32_t>(vulkanCommandBuffers.size());
		info.pCommandBufferInfos = vulkanCommandBuffers.data();
		info.signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreInfos.size());
		info.pSignalSemaphoreInfos = signalSemaphoreInfos.data();
		
		VkFence waitFence = nullptr;

		if (executeInfo.fence)
		{
			VulkanFence& vulkanFence = executeInfo.fence->AsRef<VulkanFence>();
			waitFence = executeInfo.fence->GetHandle<VkFence>();
			vulkanFence.MarkAsExecuted();
		}

		// Assign the semaphore value here, to make sure the execution order is correct.
		uint64_t submitSemaphoreValue;
		{
			std::scoped_lock lock{ m_executeMutex };
			submitSemaphoreValue = m_semaphoreValue++;

			for (auto& signalSemaphoreInfo : signalSemaphoreInfos)
			{
				signalSemaphoreInfo.value = submitSemaphoreValue;
			}

 			VT_VK_CHECK(vkQueueSubmit2(m_queue, 1, &info, waitFence));
		}

		if (executeInfo.fence_new)
		{
			VulkanFence_New& vkFence = executeInfo.fence_new->AsRef<VulkanFence_New>();
			vkFence.m_referencedSemaphore = m_queueSemaphore;
			vkFence.m_referencedValue = submitSemaphoreValue;
		}

		for (const auto& cmdBuffer : executeInfo.commandBuffers)
		{
			VulkanCommandBuffer& vkCmdBuffer = cmdBuffer->AsRef<VulkanCommandBuffer>();
			VulkanFence_New& vkSubmissionFence = vkCmdBuffer.m_submissionFence->AsRef<VulkanFence_New>();
			vkSubmissionFence.m_referencedValue = submitSemaphoreValue;
			vkSubmissionFence.m_referencedSemaphore = m_queueSemaphore;
		}
	}

	void VulkanDeviceQueue::AquireLock()
	{
		m_executeMutex.lock();
	}

	void VulkanDeviceQueue::ReleaseLock()
	{
		m_executeMutex.unlock();
	}

	void* VulkanDeviceQueue::GetHandleImpl() const
	{
		return m_queue;
	}

	void VulkanDeviceQueue::CreateQueueSemaphore(VulkanGraphicsDevice& graphicsDevice)
	{
		VkSemaphoreTypeCreateInfo semaphoreTypeInfo;
		semaphoreTypeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		semaphoreTypeInfo.pNext = nullptr;
		semaphoreTypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		semaphoreTypeInfo.initialValue = 0;

		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		createInfo.pNext = &semaphoreTypeInfo;
		createInfo.flags = 0;

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateSemaphore(graphicsDevice.GetHandle<VkDevice>(), &createInfo, VT_VULKAN_ALLOCATOR, &m_queueSemaphore));
	}

	void VulkanDeviceQueue::DestroyQueueSemaphore(class VulkanGraphicsDevice& graphicsDevice)
	{
		vkDestroySemaphore(graphicsDevice.GetHandle<VkDevice>(), m_queueSemaphore, VT_VULKAN_ALLOCATOR);
	}
}
