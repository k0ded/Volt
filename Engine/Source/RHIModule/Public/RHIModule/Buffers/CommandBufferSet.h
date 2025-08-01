#pragma once

#include "RHIModule/Buffers/CommandBuffer.h"
#include "RHIModule/Synchronization/Fence.h"

namespace Volt::RHI
{
	class VTRHI_API CommandBufferSet
	{
	public:
		CommandBufferSet(const uint32_t count, QueueType queueType = QueueType::Graphics);
		CommandBufferSet(const CommandBufferSet& other) noexcept;
		CommandBufferSet(CommandBufferSet&& other) noexcept;

		CommandBufferSet& operator=(const CommandBufferSet& other) noexcept;
		CommandBufferSet& operator=(CommandBufferSet&& other) noexcept;

		RefPtr<CommandBuffer> GetCurrentCommandBuffer() const;
		RefPtr<Fence> GetCurrentFence() const;
		RefPtr<CommandBuffer> IncrementAndGetCommandBuffer();
		void Increment();

		VT_NODISCARD VT_INLINE uint32_t GetCurrentIndex() const { return m_currentIndex; }

	private:
		Vector<RefPtr<CommandBuffer>> m_commandBuffers;
		Vector<RefPtr<Fence>> m_fences;
		
		uint32_t m_currentIndex = 0;
		uint32_t m_count;
	};
}
