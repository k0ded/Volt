#include "vkpch.h"
#include "VulkanRHIModule/Graphics/VulkanDeviceQueue.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"

#include "VulkanRHIModule/Graphics/VulkanGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"
#include "VulkanRHIModule/Synchronization/VulkanFence.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Synchronization/Semaphore.h>
#include <RHIModule/RHIModule.h>

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
		VT_ENSURE_MSG(RHIModule::GetSubmissionThread().GetSubmissionThreadId() == std::this_thread::get_id(), "Submissions may only come from the RHI Submission Thread!");

		GlobalMemoryStackMark memMark;

		GlobalMemoryStackVector<VkCommandBufferSubmitInfo> vulkanCommandBuffers;
		vulkanCommandBuffers.reserve(executeInfo.commandBuffers.size());

		GlobalMemoryStackVector<VkSemaphoreSubmitInfo> signalSemaphoreInfos;
		signalSemaphoreInfos.reserve(executeInfo.signalFences.size());

		size_t numWaitSemaphores = executeInfo.waitSemaphores.size();

		for (const auto& cmdBuffer : executeInfo.commandBuffers)
		{
			VulkanCommandBuffer& vkCmdBuffer = cmdBuffer->AsRef<VulkanCommandBuffer>();
			numWaitSemaphores += vkCmdBuffer.m_waitSemaphores.size();
		}

		GlobalMemoryStackVector<VkSemaphoreSubmitInfo> waitSemaphoreInfos;
		waitSemaphoreInfos.reserve(numWaitSemaphores);

		for (const auto& cmdBuffer : executeInfo.commandBuffers)
		{
			VulkanCommandBuffer& vkCmdBuffer = cmdBuffer->AsRef<VulkanCommandBuffer>();
			
			vkCmdBuffer.m_submissionFence = Fence::Create();

			auto& info = vulkanCommandBuffers.emplace_back();
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
			info.pNext = nullptr;
			info.deviceMask = 0;
			info.commandBuffer = vkCmdBuffer.GetHandle<VkCommandBuffer>();

			for (VkSemaphore semaphore : vkCmdBuffer.m_waitSemaphores)
			{
				auto& waitSemaphoreInfo = waitSemaphoreInfos.emplace_back();
				waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
				waitSemaphoreInfo.pNext = nullptr;
				waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_NONE;
				waitSemaphoreInfo.semaphore = semaphore;
				waitSemaphoreInfo.deviceIndex = 0;
				waitSemaphoreInfo.value = 0;
			}
		}

		for (const auto& fence : executeInfo.signalFences)
		{
			auto& info = signalSemaphoreInfos.emplace_back();
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			info.pNext = nullptr;
			info.deviceIndex = 0;
			info.stageMask = VK_PIPELINE_STAGE_2_NONE;
			info.semaphore = fence->GetHandle<VkSemaphore>();
		}

		{
			auto& queueSemaphore = signalSemaphoreInfos.emplace_back();
			queueSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			queueSemaphore.pNext = nullptr;
			queueSemaphore.deviceIndex = 0;
			queueSemaphore.stageMask = VK_PIPELINE_STAGE_2_NONE;
			queueSemaphore.semaphore = m_queueSemaphore;
		}

		for (const IntRef<Semaphore>& waitSemaphore : executeInfo.waitSemaphores)
		{
			auto& info = waitSemaphoreInfos.emplace_back();
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			info.pNext = nullptr;
			info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
			info.semaphore = waitSemaphore->GetHandle<VkSemaphore>();
			info.deviceIndex = 0;
			info.value = 0;
		}

		VkSubmitInfo2 info{};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		info.commandBufferInfoCount = static_cast<uint32_t>(vulkanCommandBuffers.size());
		info.pCommandBufferInfos = vulkanCommandBuffers.data();
		info.signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreInfos.size());
		info.pSignalSemaphoreInfos = signalSemaphoreInfos.data();
		info.waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphoreInfos.size());
		info.pWaitSemaphoreInfos = waitSemaphoreInfos.data();
		
		// Assign the semaphore value here, to make sure the execution order is correct.
		uint64_t submitSemaphoreValue;
		{
			std::scoped_lock lock{ m_executeMutex };
			VT_PROFILE_LOCK_MARK(m_executeMutex);

			submitSemaphoreValue = m_semaphoreValue++;

			for (auto& signalSemaphoreInfo : signalSemaphoreInfos)
			{
				signalSemaphoreInfo.value = submitSemaphoreValue;
			}

 			VT_VK_CHECK(vkQueueSubmit2(m_queue, 1, &info, nullptr));
		}

		for (const auto& fence : executeInfo.signalFences)
		{
			VulkanFence& vkFence = fence->AsRef<VulkanFence>();
			vkFence.AssignSemaphore(m_queueSemaphore, submitSemaphoreValue);
		}

		if (executeInfo.executionFence)
		{
			VulkanFence& vkFence = executeInfo.executionFence->AsRef<VulkanFence>();
			vkFence.AssignSemaphore(m_queueSemaphore, submitSemaphoreValue);
		}

		for (const auto& cmdBuffer : executeInfo.commandBuffers)
		{
			VulkanCommandBuffer& vkCmdBuffer = cmdBuffer->AsRef<VulkanCommandBuffer>();
			vkCmdBuffer.AssignSemaphore(m_queueSemaphore, submitSemaphoreValue);
		}
	}

	void VulkanDeviceQueue::AquireLock()
	{
		m_executeMutex.lock();
		VT_PROFILE_LOCK_MARK(m_executeMutex);
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

		VT_VK_CHECK(vkCreateSemaphore(graphicsDevice.GetHandle<VkDevice>(), &createInfo, VT_VULKAN_ALLOCATOR, &m_queueSemaphore));
	}

	void VulkanDeviceQueue::DestroyQueueSemaphore(class VulkanGraphicsDevice& graphicsDevice)
	{
		vkDestroySemaphore(graphicsDevice.GetHandle<VkDevice>(), m_queueSemaphore, VT_VULKAN_ALLOCATOR);
	}

	void VulkanDeviceQueue::SwapchainExecute(VkSemaphore_T* presentSemaphore, VkSemaphore_T* renderSemaphore, VkFence_T* renderFence, VkCommandBuffer_T* commandBuffer)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE_MSG(RHIModule::GetSubmissionThread().GetSubmissionThreadId() == std::this_thread::get_id(), "Submissions may only come from the RHI Submission Thread!");

		GlobalMemoryStackMark memMark;

		VkSemaphoreSubmitInfo presentWaitInfo{};
		presentWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		presentWaitInfo.pNext = nullptr;
		presentWaitInfo.semaphore = presentSemaphore;
		presentWaitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		presentWaitInfo.deviceIndex = 0;
		presentWaitInfo.value = 1;

		VkSemaphoreSubmitInfo signalInfo{};
		signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalInfo.pNext = nullptr;
		signalInfo.semaphore = renderSemaphore;
		signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		signalInfo.deviceIndex = 0;
		signalInfo.value = 1;

		VkCommandBufferSubmitInfo cmdBufferInfo{};
		cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		cmdBufferInfo.pNext = nullptr;
		cmdBufferInfo.commandBuffer = commandBuffer;
		cmdBufferInfo.deviceMask = 0;

		VkSubmitInfo2 submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.pNext = nullptr;
		submitInfo.flags = 0;
		submitInfo.waitSemaphoreInfoCount = presentSemaphore != nullptr ? 1 : 0;
		submitInfo.pWaitSemaphoreInfos = &presentWaitInfo;
		submitInfo.signalSemaphoreInfoCount = 1;
		submitInfo.pSignalSemaphoreInfos = &signalInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &cmdBufferInfo;

		{
			std::scoped_lock lock{ m_executeMutex };
			VT_PROFILE_LOCK_MARK(m_executeMutex);

			VT_VK_CHECK(vkQueueSubmit2(m_queue, 1, &submitInfo, renderFence));
		}
	}

	void VulkanDeviceQueue::SwapchainPresent(VkSwapchainKHR_T* swapchain, VkSemaphore_T* renderSemaphore, VkFence_T* presentFence, uint32_t imageIndex, std::mutex* swapchainMutex)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE_MSG(RHIModule::GetSubmissionThread().GetSubmissionThreadId() == std::this_thread::get_id(), "Submissions may only come from the RHI Submission Thread!");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pWaitSemaphores = &renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pImageIndices = &imageIndex;

		VkSwapchainPresentFenceInfoEXT fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
		fenceInfo.pNext = nullptr;
		fenceInfo.pFences = &presentFence;
		fenceInfo.swapchainCount = 1;

		presentInfo.pNext = &fenceInfo;

		{
			std::scoped_lock lock{ m_executeMutex };
			VT_PROFILE_LOCK_MARK(m_executeMutex);

			swapchainMutex->lock();
			VT_MAYBE_UNUSED VkResult presentResult = vkQueuePresentKHR(m_queue, &presentInfo);
			swapchainMutex->unlock();

			VT_ENSURE(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || presentResult == VK_SUCCESS);
		}
	}
}
