#include "rhipch.h"
#include "RHIModule/Buffers/CommandBufferUtility.h"

#include "RHIModule/Graphics/GraphicsContext.h"
#include "RHIModule/Graphics/GraphicsDevice.h"
#include "RHIModule/Graphics/DeviceQueue.h"

namespace Volt::RHI::CommandBufferUtils
{
	RefPtr<Fence_New> ExecuteCommandBufferWithNewFence(RefPtr<CommandBuffer> commandBuffer, QueueType queueType)
	{
		RefPtr<Fence_New> fence = Fence_New::Create();

		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.fence_new = fence;

		GraphicsContext::GetDevice()->GetDeviceQueue(queueType)->Execute(executeInfo);

		return fence;
	}

	void ExecuteCommandBufferWithFence(RefPtr<CommandBuffer> commandBuffer, RefPtr<Fence_New> fence, QueueType queueType)
	{
		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.fence_new = fence;

		GraphicsContext::GetDevice()->GetDeviceQueue(queueType)->Execute(executeInfo);
	}

	RefPtr<Fence_New> ExecuteCommandBufferWithNewFenceAndWait(RefPtr<CommandBuffer> commandBuffer, QueueType queueType /*= QueueType::Graphics*/)
	{
		RefPtr<Fence_New> fence = Fence_New::Create();

		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { commandBuffer };
		executeInfo.fence_new = fence;

		GraphicsContext::GetDevice()->GetDeviceQueue(queueType)->Execute(executeInfo);
		fence->WaitUntilSignaled();

		return fence;
	}
}
