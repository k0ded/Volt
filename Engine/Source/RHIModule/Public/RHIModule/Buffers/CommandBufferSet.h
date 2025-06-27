#pragma once

#include "RHIModule/Buffers/CommandBuffer.h"
#include "RHIModule/Synchronization/Fence.h"

namespace Volt::RHI
{
	class VTRHI_API CommandBufferSet
	{
	public:
		CommandBufferSet(const uint32_t count, QueueType queueType = QueueType::Graphics);

		RefPtr<CommandBuffer> GetCurrentCommandBuffer() const;
		RefPtr<Fence> GetCurrentFence() const;
		RefPtr<CommandBuffer> IncrementAndGetCommandBuffer();
		void Increment();

	private:
		Vector<RefPtr<CommandBuffer>> m_commandBuffers;
		Vector<RefPtr<Fence>> m_fences;
		
		uint32_t m_currentIndex = 0;
		const uint32_t m_count;
	};
}
