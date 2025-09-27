#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Synchronization/Fence_New.h"
#include "RHIModule/Buffers/CommandBuffer.h"

namespace Volt::RHI::CommandBufferUtils
{
	extern VTRHI_API RefPtr<Fence_New> ExecuteCommandBufferWithNewFence(RefPtr<CommandBuffer> commandBuffer, QueueType queueType = QueueType::Graphics);
	extern VTRHI_API RefPtr<Fence_New> ExecuteCommandBufferWithNewFenceAndWait(RefPtr<CommandBuffer> commandBuffer, QueueType queueType = QueueType::Graphics);
	extern VTRHI_API void ExecuteCommandBufferWithFence(RefPtr<CommandBuffer> commandBuffer, RefPtr<Fence_New> fence, QueueType queueType = QueueType::Graphics);
}
