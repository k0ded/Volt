#include "rcpch.h"

#include "RenderCore/CommandBufferPool.h"

#include <RHIModule/Graphics/Swapchain.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	CommandBufferPool::CommandBufferPool()
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(!s_instance);
		s_instance = this;

		m_waitingCommandBufferPool.resize(RHI::Swapchain::FramesInFlight);
		for (uint32_t i = 0; i < RHI::Swapchain::FramesInFlight; ++i)
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

		m_frameIndex = ++m_frameIndex % RHI::Swapchain::FramesInFlight;
		
		RefPtr<RHI::CommandBuffer> commandBuffer;
		while (m_waitingCommandBufferPool.at(m_frameIndex).Pop(commandBuffer))
		{
			m_commandBufferPool.Push(commandBuffer);
		}
	}

	RefPtr<RHI::CommandBuffer> CommandBufferPool::GetCommandBuffer()
	{
		VT_PROFILE_FUNCTION();
		// We try to pop a command buffer from the stack.
		RefPtr<RHI::CommandBuffer> result;
		if (s_instance->m_commandBufferPool.Pop(result))
		{
			return result;
		}

		// If no command buffers were available, we fallback to creating a new one.
		result = RHI::CommandBuffer::Create();
		return result;
	}

	void CommandBufferPool::FreeCommandBuffer(RefPtr<RHI::CommandBuffer> commandBuffer)
	{
		VT_PROFILE_FUNCTION();
		s_instance->m_waitingCommandBufferPool.at(s_instance->m_frameIndex).Push(commandBuffer);
	}

	void CommandBufferPool::CreateInitialCommandBuffers()
	{
		constexpr size_t NumInitialCommandBuffers = 1024;

		for (size_t i = 0; i < NumInitialCommandBuffers; i++)
		{
			m_commandBufferPool.Push(RHI::CommandBuffer::Create());
		}
	}
}
