#include "rhipch.h"
#include "RHIModule/Buffers/CommandBufferUtility.h"

#include "RHIModule/Graphics/GraphicsContext.h"
#include "RHIModule/Graphics/GraphicsDevice.h"
#include "RHIModule/Graphics/DeviceQueue.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI::CommandBufferUtils
{
	RefPtr<Fence> ExecuteCommandBufferWithNewFence(RefPtr<CommandBuffer> commandBuffer, QueueType queueType)
	{
		RefPtr<Fence> fence = Fence::Create();

		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.executionFence = fence;

		RHIModule::GetSubmissionThread().QueueSubmit(std::move(executeInfo), queueType);

		return fence;
	}

	void ExecuteCommandBufferWithFence(RefPtr<CommandBuffer> commandBuffer, RefPtr<Fence> fence, QueueType queueType)
	{
		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.executionFence = fence;

		RHIModule::GetSubmissionThread().QueueSubmit(std::move(executeInfo), queueType);
	}

	RefPtr<Fence> ExecuteCommandBufferWithNewFenceAndWait(RefPtr<CommandBuffer> commandBuffer, QueueType queueType /*= QueueType::Graphics*/)
	{
		RefPtr<Fence> fence = Fence::Create();

		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.executionFence = fence;

		RHIModule::GetSubmissionThread().QueueSubmit(std::move(executeInfo), queueType);
		fence->WaitUntilSignaled();

		return fence;
	}
}
