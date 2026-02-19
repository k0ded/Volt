#include "rcpch.h"

#include "RenderCore/Resources/GrowingGPUBuffer.h"
#include "RenderCore/CommandBufferPool.h"

#include <RHIModule/Buffers/Buffer.h>
#include <RHIModule/Core/ResourceStateTracker.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>

namespace Volt
{
	GrowingGPUBuffer::GrowingGPUBuffer(uint32_t initialCount, uint64_t elementSize, const std::string& name, RHI::BufferUsage bufferUsage, RHI::MemoryUsage memoryUsage)
	{
		RHI::BufferDesc desc{};
		desc.numElements = initialCount;
		desc.elementSize = elementSize;
		desc.debugName = name;
		desc.usage = bufferUsage;
		desc.memoryUsage = memoryUsage;

		m_buffer = RHI::Buffer::Create(desc);
	}

	GrowingGPUBuffer::~GrowingGPUBuffer()
	{
		m_buffer = nullptr;
	}

	void GrowingGPUBuffer::GrowIfRequired(uint32_t requestedElementCount)
	{
		constexpr float GrowMultiplier = 1.5f;

		if (m_buffer->GetNumElements() < requestedElementCount)
		{
			RHI::BufferDesc bufferDesc = m_buffer->GetDesc();
			bufferDesc.numElements = std::max(static_cast<uint32_t>(bufferDesc.numElements * GrowMultiplier), requestedElementCount);

			RefPtr<RHI::Buffer> tempBuffer = RHI::Buffer::Create(bufferDesc);

			// #TODO_Ivar: Make this optional
			// Copy previous contents into new buffer.
			{
				RefPtr<PooledCommandBuffer> pooledCommandBuffer = CommandBufferPool::GetCommandBuffer();
				RefPtr<RHI::CommandBuffer> commandBuffer = pooledCommandBuffer->Get();

				commandBuffer->Begin();

				const auto& currentResourceState = RHI::GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(m_buffer, 0);

				{
					RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsGlobalBarrier();
					barrier.globalBarrier().srcStage = currentResourceState.stage;
					barrier.globalBarrier().srcAccess = currentResourceState.access;
					barrier.globalBarrier().dstStage = RHI::BarrierStage::Copy;
					barrier.globalBarrier().dstAccess = RHI::BarrierAccess::CopySource | RHI::BarrierAccess::CopyDest;

					commandBuffer->ResourceBarrier({ barrier });
				}

				commandBuffer->CopyBufferRegion(m_buffer, 0, tempBuffer, 0, m_buffer->GetMemoryRequirements().size);

				{
					RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsGlobalBarrier();
					barrier.globalBarrier().srcStage = RHI::BarrierStage::Copy;
					barrier.globalBarrier().srcAccess = RHI::BarrierAccess::CopySource | RHI::BarrierAccess::CopyDest;
					barrier.globalBarrier().dstStage = currentResourceState.stage;
					barrier.globalBarrier().dstAccess = currentResourceState.access;

					commandBuffer->ResourceBarrier({ barrier });
				}

				RHI::GraphicsContext::GetResourceStateTracker()->TransitionResource(tempBuffer, 0, currentResourceState.stage, currentResourceState.access, currentResourceState.layout);
				commandBuffer->End();
				RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFenceAndWait(commandBuffer);
			}

			m_buffer = tempBuffer;
		}
	}

	void GrowingGPUBuffer::GrowIfRequired(uint64_t requestedElementCount)
	{
		GrowIfRequired(static_cast<uint32_t>(requestedElementCount));
	}

	uint64_t GrowingGPUBuffer::GetByteSize()
	{
		const RHI::BufferDesc& bufferDesc = m_buffer->GetDesc();
		return bufferDesc.numElements * bufferDesc.elementSize;
	}

	RefPtr<RHI::Buffer> GrowingGPUBuffer::GetResource() const
	{
		return m_buffer;
	}
}
