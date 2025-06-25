#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Synchronization/Fence.h"
#include "RHIModule/Buffers/CommandBuffer.h"

namespace Volt::RHI::CommandBufferUtils
{
	extern VTRHI_API RefPtr<Fence> ExecuteCommandBufferWithNewFence(RefPtr<CommandBuffer> commandBuffer, QueueType queueType = QueueType::Graphics);
	extern VTRHI_API RefPtr<Fence> ExecuteCommandBufferWithNewFenceAndWait(RefPtr<CommandBuffer> commandBuffer, QueueType queueType = QueueType::Graphics);
	extern VTRHI_API void ExecuteCommandBufferWithFence(RefPtr<CommandBuffer> commandBuffer, RefPtr<Fence> fence, QueueType queueType = QueueType::Graphics);
}
