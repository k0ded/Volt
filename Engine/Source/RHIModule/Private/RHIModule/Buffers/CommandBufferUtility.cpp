#include "rhipch.h"
#include "RHIModule/Buffers/CommandBufferUtility.h"

#include "RHIModule/Graphics/GraphicsContext.h"
#include "RHIModule/Graphics/GraphicsDevice.h"
#include "RHIModule/Graphics/DeviceQueue.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI::CommandBufferUtils
{
	IntRef<Fence> ExecuteCommandBufferWithNewFence(IntRef<CommandBuffer> commandBuffer, QueueType queueType)
	{
		IntRef<Fence> fence = Fence::Create();

		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.executionFence = fence;

		RHIModule::GetSubmissionThread().QueueSubmit(std::move(executeInfo), queueType);

		return fence;
	}

	void ExecuteCommandBufferWithFence(IntRef<CommandBuffer> commandBuffer, IntRef<Fence> fence, QueueType queueType)
	{
		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.executionFence = fence;

		RHIModule::GetSubmissionThread().QueueSubmit(std::move(executeInfo), queueType);
	}

	IntRef<Fence> ExecuteCommandBufferWithNewFenceAndWait(IntRef<CommandBuffer> commandBuffer, QueueType queueType /*= QueueType::Graphics*/)
	{
		IntRef<Fence> fence = Fence::Create();

		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.executionFence = fence;

		RHIModule::GetSubmissionThread().QueueSubmit(std::move(executeInfo), queueType);
		fence->WaitUntilSignaled();

		return fence;
	}
}
