#include "rhipch.h"

#include "RHIModule/Buffers/BufferUtility.h"
#include "RHIModule/Buffers/CommandBuffer.h"
#include "RHIModule/Buffers/CommandBufferUtility.h"
#include "RHIModule/Graphics/GraphicsContext.h"

namespace Volt::RHI::BufferUtility
{
	void StagedBufferUpload(RefPtr<RHI::Buffer> buffer, const void* data, uint64_t size)
	{
		RHI::BufferDesc stagingBufferDesc{};
		stagingBufferDesc.elementSize = 1;
		stagingBufferDesc.numElements = size;
		stagingBufferDesc.memoryUsage = MemoryUsage::CPUToGPU;
		stagingBufferDesc.usage = BufferUsage::TransferSrc | BufferUsage::StorageBuffer;
		stagingBufferDesc.debugName = "Staging Buffer";

		RefPtr<RHI::Buffer> stagingBuffer = RHI::Buffer::Create(stagingBufferDesc);

		{
			void* mappedPtr = stagingBuffer->Map<void>();
			memcpy_s(mappedPtr, size, data, size);
			stagingBuffer->Unmap();
		}

		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();

		commandBuffer->Begin();

		const auto& currentState = RHI::GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(buffer, 0);

		{
			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsGlobalBarrier();
			barrier.globalBarrier().srcAccess = currentState.access;
			barrier.globalBarrier().srcStage = currentState.stage;
			barrier.globalBarrier().dstStage = RHI::BarrierStage::Copy;
			barrier.globalBarrier().dstAccess = RHI::BarrierAccess::CopyDest;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->CopyBufferRegion(stagingBuffer, 0, buffer, 0, size);

		{
			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsGlobalBarrier();
			barrier.globalBarrier().dstAccess = currentState.access;
			barrier.globalBarrier().dstStage = currentState.stage;
			barrier.globalBarrier().srcStage = RHI::BarrierStage::Copy;
			barrier.globalBarrier().srcAccess = RHI::BarrierAccess::CopyDest;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->End();

		RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
	}

	RefPtr<Volt::RHI::Buffer> ResizeBufferIfRequired(RefPtr<RHI::Buffer> buffer, uint64_t numElements)
	{
		if (buffer->GetNumElements() >= numElements)
		{
			return buffer;
		}

		RHI::BufferDesc bufferDesc = buffer->GetDesc();
		bufferDesc.numElements = numElements;

		return RHI::Buffer::Create(bufferDesc);
	}
}
