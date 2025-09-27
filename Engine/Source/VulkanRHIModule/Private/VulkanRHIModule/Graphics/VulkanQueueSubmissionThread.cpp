#include "vkpch.h"

#include "VulkanRHIModule/Graphics/VulkanQueueSubmissionThread.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

#include <Volt-Platforms/Platform.h>

namespace Volt::RHI
{

	VulkanQueueSubmissionThread::VulkanQueueSubmissionThread()
	{
		m_isRunning = true;
		m_submissionQueue.Allocate(1024);
		m_submissionThread = CreateScope<std::thread>(std::bind(&VulkanQueueSubmissionThread::Run, this));
		PlatformThread::SetThreadName(m_submissionThread->native_handle(), "VulkanQueueSubmissionThread");
	}

	VulkanQueueSubmissionThread::~VulkanQueueSubmissionThread()
	{
		m_isRunning = false;
		m_submissionThread->join();
		m_submissionThread = nullptr;
	}

	void VulkanQueueSubmissionThread::Run()
	{
		VkSubmitInfo2 vkSubmitInfo{};
		vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		vkSubmitInfo.pNext = nullptr;

		VkPresentInfoKHR vkPresentInfo{};
		vkPresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		vkPresentInfo.pNext = nullptr;

		while (m_isRunning)
		{
			SubmissionInfo submissionInfo;
			while (m_submissionQueue.Pop(submissionInfo))
			{
				if (!submissionInfo.isQueuePresent)
				{
					vkSubmitInfo.commandBufferInfoCount = static_cast<uint32_t>(submissionInfo.commandBuffers.size());
					vkSubmitInfo.pCommandBufferInfos = submissionInfo.commandBuffers.data();
					vkSubmitInfo.signalSemaphoreInfoCount = static_cast<uint32_t>(submissionInfo.signalSemaphoreInfos.size());
					vkSubmitInfo.pSignalSemaphoreInfos = submissionInfo.signalSemaphoreInfos.data();
					vkSubmitInfo.waitSemaphoreInfoCount = static_cast<uint32_t>(submissionInfo.waitSemaphoreInfos.size());
					vkSubmitInfo.pWaitSemaphoreInfos = submissionInfo.waitSemaphoreInfos.data();

					VT_VK_CHECK(vkQueueSubmit2(submissionInfo.queue, 1, &vkSubmitInfo, submissionInfo.fence));
				}
				else
				{
					vkPresentInfo.swapchainCount = 1;
					vkPresentInfo.pSwapchains = &submissionInfo.swapchain;
					vkPresentInfo.waitSemaphoreCount = 1;
					vkPresentInfo.pWaitSemaphores = &submissionInfo.presentWaitSemaphore;
					vkPresentInfo.pImageIndices = &submissionInfo.imageIndex;

					if (submissionInfo.swapchainMutex)
					{
						submissionInfo.swapchainMutex->lock();
					}

					vkQueuePresentKHR(submissionInfo.queue, &vkPresentInfo);

					if (submissionInfo.swapchainMutex)
					{
						submissionInfo.swapchainMutex->unlock();
					}
				}
			}

			std::unique_lock<std::mutex> lock(m_wakeMutex);
			m_wakeCondition.wait(lock);
		}
	}

	void VulkanQueueSubmissionThread::QueueSubmission(const SubmissionInfo& submissionInfo)
	{
		VT_ENSURE_MSG(submissionInfo.queue, "Queue must be valid!");

		m_submissionQueue.Emplace(submissionInfo);
		m_wakeCondition.notify_all();
	}
}
