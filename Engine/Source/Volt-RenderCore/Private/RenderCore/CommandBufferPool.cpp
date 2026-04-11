#include "rcpch.h"

#include "RenderCore/CommandBufferPool.h"

#include <RHIModule/RHICapabilities.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	CommandBufferPool::CommandBufferPool()
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(!s_instance);
		s_instance = this;

		m_waitingCommandBufferPool.resize(RHI::RHICapabilities::NumFramesInFlight);
		for (uint32_t i = 0; i < RHI::RHICapabilities::NumFramesInFlight; ++i)
		{
			m_waitingCommandBufferPool[i].Allocate(WaitCommandBufferPoolSize);
		}

		m_commandBufferPool.Allocate(CommandBufferPoolSize);

		CreateInitialCommandBuffers();
	}

	CommandBufferPool::~CommandBufferPool()
	{
		s_instance = nullptr;
	}

	void CommandBufferPool::Update()
	{
		VT_PROFILE_FUNCTION();

		m_frameIndex = ++m_frameIndex % RHI::RHICapabilities::NumFramesInFlight;
		uint32_t nextFrameIndex = (m_frameIndex + 1) % RHI::RHICapabilities::NumFramesInFlight;

		IntRef<RHI::CommandBuffer> commandBuffer;
		while (m_waitingCommandBufferPool.at(m_frameIndex).Pop(commandBuffer))
		{
			if (commandBuffer->HasFinishedExecution())
			{
				commandBuffer->Reset();
				m_commandBufferPool.Push(commandBuffer);
			}
			else
			{
				m_waitingCommandBufferPool.at(nextFrameIndex).Push(commandBuffer);
			}
		}
	}

	IntRef<PooledCommandBuffer> CommandBufferPool::GetCommandBuffer()
	{
		VT_PROFILE_FUNCTION();
		// We try to pop a command buffer from the stack.
		IntRef<RHI::CommandBuffer> result;
		if (s_instance->m_commandBufferPool.Pop(result))
		{
			return IntRef<PooledCommandBuffer>::Create(result);
		}

		// If no command buffers were available, we fallback to creating a new one.
		result = RHI::CommandBuffer::Create();
		return IntRef<PooledCommandBuffer>::Create(result);
	}

	void CommandBufferPool::FreeCommandBuffer(IntRef<RHI::CommandBuffer> commandBuffer)
	{
		VT_PROFILE_FUNCTION();
		VT_MAYBE_UNUSED bool succeded = s_instance->m_waitingCommandBufferPool.at(s_instance->m_frameIndex).Push(commandBuffer);
		VT_ENSURE(succeded);
	}

	void CommandBufferPool::CreateInitialCommandBuffers()
	{
		constexpr size_t NumInitialCommandBuffers = 1024;

		for (size_t i = 0; i < NumInitialCommandBuffers; i++)
		{
			m_commandBufferPool.Push(RHI::CommandBuffer::Create());
		}
	}

	PooledCommandBuffer::~PooledCommandBuffer()
	{
		CommandBufferPool::FreeCommandBuffer(m_commandBuffer);
	}

	PooledCommandBuffer::PooledCommandBuffer(IntRef<RHI::CommandBuffer> commandBuffer)
	{
		m_commandBuffer = commandBuffer;
	}
}
