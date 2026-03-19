#pragma once

#include <RHIModule/RHISubmissionThread.h>
#include <RHIModule/Graphics/DeviceQueue.h>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Variant.h>
#include <CoreUtilities/Pointers/Unique.h>

struct VkSemaphore_T;
struct VkFence_T;
struct VkCommandBuffer_T;
struct VkSwapchainKHR_T;

namespace Volt::RHI
{
	class VulkanRHISubmissionThread : public RHISubmissionThread
	{
	public:
		VulkanRHISubmissionThread();
		~VulkanRHISubmissionThread() override;

		void QueueSubmit(DeviceQueueExecuteInfo&& executeInfo, QueueType queueType) override;
		std::thread::id GetSubmissionThreadId() const override;

		void QueueSwapchainSubmit(
			VkSemaphore_T* presentSemaphore,
			VkSemaphore_T* renderSemaphore,
			VkFence_T* renderFence,
			VkCommandBuffer_T* commandBuffer);

		void QueueSwapchainPresent(
			VkSwapchainKHR_T* swapchain,
			VkSemaphore_T* renderSemaphore,
			uint32_t imageIndex,
			std::mutex* swapchainMutex
		);

	private:
		enum class SubmissionType : uint8_t
		{
			Normal,
			SwapchainSubmit,
			SwapchianPresent
		};

		struct SubmissionData
		{
			struct SwapchainSubmit
			{
				VkSemaphore_T* presentSemaphore;
				VkSemaphore_T* renderSemaphore;
				VkFence_T* renderFence;
				VkCommandBuffer_T* commandBuffer;
			};

			struct SwapchainPresent
			{
				VkSwapchainKHR_T* swapchain;
				VkSemaphore_T* renderSemaphore;
				std::mutex* swapchainMutex;
				uint32_t imageIndex;
			};

			Variant<DeviceQueueExecuteInfo, SwapchainSubmit, SwapchainPresent> data;
			QueueType queueType;
			SubmissionType submissionType;
		};

		void RunSubmissionThread();
		void MarkFencesAsSubmitted(DeviceQueueExecuteInfo& executeInfo);

		std::atomic_bool m_isRunning = true;
		std::condition_variable_any m_wakeCondition;
		VT_PROFILE_DECLARE_MUTEX_NAMED(std::mutex, m_wakeMutex, "VulkanRHISubmission Mutex");
		Unique<std::thread> m_thread;

		WorkQueue<SubmissionData, QueueThreadingPolicy::MPSC> m_submissionQueue;
	};
}
