#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Buffers/CommandBuffer.h>

#include <CoreUtilities/Containers/AtomicStack.h>

namespace Volt
{
	class VTRC_API CommandBufferPool
	{
	public:
		CommandBufferPool();
		~CommandBufferPool();

		void Update();

		static RefPtr<RHI::CommandBuffer> GetCommandBuffer();
		static void FreeCommandBuffer(RefPtr<RHI::CommandBuffer> commandBuffer);

	private:
		inline static CommandBufferPool* s_instance = nullptr;

		void CreateInitialCommandBuffers();

		inline static constexpr size_t CommandBufferPoolSize = 4096;
		inline static constexpr size_t WaitCommandBufferPoolSize = 1024;

		AtomicStack<RefPtr<RHI::CommandBuffer>, CommandBufferPoolSize> m_commandBufferPool;
		Vector<AtomicStack<RefPtr<RHI::CommandBuffer>, WaitCommandBufferPoolSize>> m_waitingCommandBufferPool;

		uint32_t m_frameIndex = 0;
	};
}
