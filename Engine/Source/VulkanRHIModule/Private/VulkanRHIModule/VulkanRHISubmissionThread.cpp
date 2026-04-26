#include "vkpch.h"

#include "VulkanRHIModule/VulkanRHISubmissionThread.h"
#include "VulkanRHIModule/Graphics/VulkanDeviceQueue.h"

#include "VulkanRHIModule/VulkanResourceCast.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <PlatformsModule/Platform.h>

#include <CoreUtilities/ThreadConfig.h>

namespace Volt::RHI
{
	VulkanRHISubmissionThread::VulkanRHISubmissionThread()
	{
		m_submissionQueue.Allocate(2048);

		m_thread = CreateUnique<std::thread>(std::bind(&VulkanRHISubmissionThread::RunSubmissionThread, this));
		PlatformThread::SetThreadName(m_thread->native_handle(), "RHI Submission Thread");
		PlatformThread::SetThreadPriority(m_thread->native_handle(), ThreadPriority::High);
	}

	VulkanRHISubmissionThread::~VulkanRHISubmissionThread()
	{
		VT_ASSERT(!m_isRunning);
	}

	void VulkanRHISubmissionThread::QueueSubmit(DeviceQueueExecuteInfo&& executeInfo, QueueType queueType)
	{
		SubmissionData submissionData{};
		submissionData.data.Emplace<DeviceQueueExecuteInfo>(executeInfo);
		submissionData.queueType = queueType;
		submissionData.submissionType = SubmissionType::Normal;

		MarkFencesAsSubmitted(submissionData.data.Get<DeviceQueueExecuteInfo>());

		VT_MAYBE_UNUSED bool succeded = m_submissionQueue.Emplace(std::move(submissionData));
		VT_ENSURE(succeded);

		m_workAvailableSemaphore.release();
	}

	std::thread::id VulkanRHISubmissionThread::GetSubmissionThreadId() const
	{
		return m_thread->get_id();
	}

	void VulkanRHISubmissionThread::RunSubmissionThread()
	{
		VT_PROFILE_THREAD("RHI Submission Thread");

		Threads::InitializeThreadConfig(false, false);

		while (m_isRunning.load(std::memory_order::relaxed))
		{
			VT_PROFILE_FRAME_START("RHI Submission");

			SubmissionData submissionData;
			while (m_submissionQueue.Pop(submissionData))
			{
				IntRef<RHI::DeviceQueue> deviceQueue = GraphicsContext::GetDevice()->GetDeviceQueue(submissionData.queueType);

				if (submissionData.submissionType == SubmissionType::Normal)
				{
					deviceQueue->Execute(submissionData.data.Get<DeviceQueueExecuteInfo>());
				}
				else if (submissionData.submissionType == SubmissionType::SwapchainSubmit)
				{
					const SubmissionData::SwapchainSubmit& swapchainSubmit = submissionData.data.Get<SubmissionData::SwapchainSubmit>();

					IntRef<RHI::VulkanDeviceQueue> vkDeviceQueue = ResourceCast(deviceQueue);

					vkDeviceQueue->SwapchainExecute(
						swapchainSubmit.presentSemaphore,
						swapchainSubmit.renderSemaphore,
						swapchainSubmit.renderFence,
						swapchainSubmit.commandBuffer
					);
				}
				else if (submissionData.submissionType == SubmissionType::SwapchianPresent)
				{
					const SubmissionData::SwapchainPresent& swapchainPresent = submissionData.data.Get<SubmissionData::SwapchainPresent>();

					IntRef<RHI::VulkanDeviceQueue> vkDeviceQueue = ResourceCast(deviceQueue);

					vkDeviceQueue->SwapchainPresent(
						swapchainPresent.swapchain,
						swapchainPresent.renderSemaphore,
						swapchainPresent.presentFence,
						swapchainPresent.imageIndex,
						swapchainPresent.swapchainMutex
					);
				}
				else
				{
					VT_ENSURE_NO_ENTRY();
				}
			}

			VT_PROFILE_FRAME_END("RHI Submission");

			submissionData = {};
			m_workAvailableSemaphore.acquire();
		}
	}

	void VulkanRHISubmissionThread::QueueSwapchainSubmit(
		VkSemaphore_T* presentSemaphore, 
		VkSemaphore_T* renderSemaphore, 
		VkFence_T* renderFence, 
		VkCommandBuffer_T* commandBuffer)
	{
		SubmissionData submissionData{};
		submissionData.submissionType = SubmissionType::SwapchainSubmit;
		submissionData.queueType = QueueType::Graphics;

		SubmissionData::SwapchainSubmit& submitInfo = submissionData.data.Emplace<SubmissionData::SwapchainSubmit>();
		submitInfo.presentSemaphore = presentSemaphore;
		submitInfo.renderSemaphore = renderSemaphore;
		submitInfo.renderFence = renderFence;
		submitInfo.commandBuffer = commandBuffer;

		VT_MAYBE_UNUSED bool succeded = m_submissionQueue.Emplace(std::move(submissionData));
		VT_ENSURE(succeded);

		m_workAvailableSemaphore.release();
	}

	void VulkanRHISubmissionThread::QueueSwapchainPresent(
		VkSwapchainKHR_T* swapchain, 
		VkSemaphore_T* renderSemaphore,
		VkFence_T* presentFence,
		uint32_t imageIndex,
		std::mutex* swapchainMutex)
	{
		SubmissionData submissionData{};
		submissionData.submissionType = SubmissionType::SwapchianPresent;
		submissionData.queueType = QueueType::Graphics;

		SubmissionData::SwapchainPresent& submitInfo = submissionData.data.Emplace<SubmissionData::SwapchainPresent>();
		submitInfo.swapchain = swapchain;
		submitInfo.renderSemaphore = renderSemaphore;
		submitInfo.presentFence = presentFence;
		submitInfo.imageIndex = imageIndex;
		submitInfo.swapchainMutex = swapchainMutex;

		VT_MAYBE_UNUSED bool succeded = m_submissionQueue.Emplace(std::move(submissionData));
		VT_ENSURE(succeded);

		m_workAvailableSemaphore.release();
	}

	void VulkanRHISubmissionThread::MarkFencesAsSubmitted(DeviceQueueExecuteInfo& executeInfo)
	{
		for (const auto& fence : executeInfo.signalFences)
		{
			VulkanFence& vkFence = fence->AsRef<VulkanFence>();
			vkFence.m_hasBeenSubmitted.store(true, std::memory_order::relaxed);
		}

		for (const auto& commandBuffer : executeInfo.commandBuffers)
		{
			VulkanCommandBuffer& vkCommandBuffer = commandBuffer->AsRef<VulkanCommandBuffer>();
			vkCommandBuffer.MarkAsSubmitted();
		}

		if (executeInfo.executionFence)
		{
			VulkanFence& vkFence = executeInfo.executionFence->AsRef<VulkanFence>();
			vkFence.m_hasBeenSubmitted.store(true, std::memory_order::relaxed);
		}
	}

	void VulkanRHISubmissionThread::Shutdown()
	{
		m_isRunning = false;
		m_workAvailableSemaphore.release();
		m_thread->join();
	}
}
