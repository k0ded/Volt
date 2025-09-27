#pragma once

#include <RHIModule/Graphics/DeviceQueue.h>

#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Core.h>

#include <vulkan/vulkan.h>

#include <thread>
#include <atomic>

namespace Volt::RHI
{
	struct SubmissionInfo
	{
		// Submission
		InlineVector<VkCommandBufferSubmitInfo, 64> commandBuffers;
		InlineVector<VkSemaphoreSubmitInfo, 64> signalSemaphoreInfos;
		InlineVector<VkSemaphoreSubmitInfo, 64> waitSemaphoreInfos;

		VkFence_T* fence = nullptr;

		// Present
		VkSwapchainKHR_T* swapchain;
		VkSemaphore_T* presentWaitSemaphore;
		uint32_t imageIndex;
		std::mutex* swapchainMutex = nullptr;

		// Common
		VkQueue_T* queue = nullptr;
		bool isQueuePresent = false;
	};

	class VulkanQueueSubmissionThread
	{
	public:
		VulkanQueueSubmissionThread();
		~VulkanQueueSubmissionThread();

		void QueueSubmission(const SubmissionInfo& submissionInfo);

	private:
		void Run();

		std::atomic_bool m_isRunning = false;
		std::mutex m_wakeMutex;
		std::condition_variable m_wakeCondition;

		Scope<std::thread> m_submissionThread;
		WorkQueue<SubmissionInfo, QueueThreadingPolicy::MPSC> m_submissionQueue;
	};
}
