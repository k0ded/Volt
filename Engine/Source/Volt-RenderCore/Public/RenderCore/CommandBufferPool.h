#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Buffers/CommandBuffer.h>

#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/Pointers/IntRefCounted.h>

namespace Volt
{
	class VTRC_API PooledCommandBuffer : public IntRefCounted<PooledCommandBuffer>
	{
	public:
		~PooledCommandBuffer() override;

		VT_INLINE IntRef<RHI::CommandBuffer> Get() const { return m_commandBuffer; }

	private:
		friend class IntRef<PooledCommandBuffer>;

		PooledCommandBuffer(IntRef<RHI::CommandBuffer> commandBuffer);

		IntRef<RHI::CommandBuffer> m_commandBuffer;
	};

	class VTRC_API CommandBufferPool
	{
	public:
		CommandBufferPool();
		~CommandBufferPool();

		void Update();

		// Returns a pooled command buffer, once the PooledCommandBuffer object
		// is no longer referenced, it's command buffer will be freed.
		static IntRef<PooledCommandBuffer> GetCommandBuffer();

		static void FreeCommandBuffer(IntRef<RHI::CommandBuffer> commandBuffer);

	private:
		inline static CommandBufferPool* s_instance = nullptr;

		void CreateInitialCommandBuffers();

		inline static constexpr size_t CommandBufferPoolSize = 4096;
		inline static constexpr size_t WaitCommandBufferPoolSize = 1024;

		AtomicStack<IntRef<RHI::CommandBuffer>> m_commandBufferPool;
		Vector<AtomicStack<IntRef<RHI::CommandBuffer>>> m_waitingCommandBufferPool;

		uint32_t m_frameIndex = 0;
	};
}
