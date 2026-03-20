#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Synchronization/Fence.h"
#include "RHIModule/Buffers/CommandBuffer.h"

namespace Volt::RHI::CommandBufferUtils
{
	extern VTRHI_API IntRef<Fence> ExecuteCommandBufferWithNewFence(IntRef<CommandBuffer> commandBuffer, QueueType queueType = QueueType::Graphics);
	extern VTRHI_API IntRef<Fence> ExecuteCommandBufferWithNewFenceAndWait(IntRef<CommandBuffer> commandBuffer, QueueType queueType = QueueType::Graphics);
	extern VTRHI_API void ExecuteCommandBufferWithFence(IntRef<CommandBuffer> commandBuffer, IntRef<Fence> fence, QueueType queueType = QueueType::Graphics);
}
