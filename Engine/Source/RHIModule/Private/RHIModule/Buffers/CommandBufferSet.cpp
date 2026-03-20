#include "rhipch.h"
#include "RHIModule/Buffers/CommandBufferSet.h"

namespace Volt::RHI
{
	CommandBufferSet::CommandBufferSet(const uint32_t count, QueueType queueType)
		: m_count(count)
	{
		m_commandBuffers.resize(m_count);
		m_fences.resize(m_count);

		for (uint32_t i = 0; i < m_count; i++)
		{
			m_commandBuffers[i] = CommandBuffer::Create(queueType);
			m_fences[i] = Fence::Create();
		}
	}

	CommandBufferSet::CommandBufferSet(const CommandBufferSet& other) noexcept
		: m_count(other.m_count),
		m_currentIndex(other.m_currentIndex),
		m_fences(other.m_fences),
		m_commandBuffers(other.m_commandBuffers)
	{
	}

	CommandBufferSet::CommandBufferSet(CommandBufferSet&& other) noexcept 
		: m_count(other.m_count),
		m_currentIndex(std::move(other.m_currentIndex)),
		m_fences(std::move(other.m_fences)),
		m_commandBuffers(std::move(other.m_commandBuffers))
	{
	}

	CommandBufferSet& CommandBufferSet::operator=(const CommandBufferSet& other) noexcept
	{
		m_count = other.m_count;
		m_currentIndex = other.m_currentIndex;
		m_fences = other.m_fences;
		m_commandBuffers = other.m_commandBuffers;

		return *this;
	}

	CommandBufferSet& CommandBufferSet::operator=(CommandBufferSet&& other) noexcept
	{
		m_count = other.m_count;
		m_currentIndex = std::move(other.m_currentIndex);
		m_fences = std::move(other.m_fences);
		m_commandBuffers = std::move(other.m_commandBuffers);

		return *this;
	}

	IntRef<CommandBuffer> CommandBufferSet::GetCurrentCommandBuffer() const
	{
		return m_commandBuffers.at(m_currentIndex);
	}
	
	IntRef<CommandBuffer> CommandBufferSet::IncrementAndGetCommandBuffer()
	{
		m_currentIndex = (m_currentIndex + 1) % m_count;
		return m_commandBuffers.at(m_currentIndex);
	}
	
	void CommandBufferSet::Increment()
	{
		m_currentIndex = (m_currentIndex + 1) % m_count;
	}

	IntRef<Fence> CommandBufferSet::GetCurrentFence() const
	{
		return m_fences.at(m_currentIndex);
	}
}
