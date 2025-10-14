#include "rhipch.h"
#include "RHIModule/Buffers/CommandBufferUtility.h"

#include "RHIModule/Graphics/GraphicsContext.h"
#include "RHIModule/Graphics/GraphicsDevice.h"
#include "RHIModule/Graphics/DeviceQueue.h"

namespace Volt::RHI::CommandBufferUtils
{
	RefPtr<Fence> ExecuteCommandBufferWithNewFence(RefPtr<CommandBuffer> commandBuffer, QueueType queueType)
	{
		RefPtr<Fence> fence = Fence::Create();

		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.executionFence = fence;

		GraphicsContext::GetDevice()->GetDeviceQueue(queueType)->Execute(executeInfo);

		return fence;
	}

	void ExecuteCommandBufferWithFence(RefPtr<CommandBuffer> commandBuffer, RefPtr<Fence> fence, QueueType queueType)
	{
		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.executionFence = fence;

		GraphicsContext::GetDevice()->GetDeviceQueue(queueType)->Execute(executeInfo);
	}

	RefPtr<Fence> ExecuteCommandBufferWithNewFenceAndWait(RefPtr<CommandBuffer> commandBuffer, QueueType queueType /*= QueueType::Graphics*/)
	{
		RefPtr<Fence> fence = Fence::Create();

		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.executionFence = fence;

		GraphicsContext::GetDevice()->GetDeviceQueue(queueType)->Execute(executeInfo);
		fence->WaitUntilSignaled();

		return fence;
	}
}
