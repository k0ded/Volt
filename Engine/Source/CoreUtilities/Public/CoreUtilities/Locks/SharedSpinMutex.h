#pragma once

#include "CoreUtilities/CompilerTraits.h"

#include <new>
#include <atomic>

// Reference: https://bitbucket.org/RadiantSoftware/engine/src/master/Core/Concurrency/SharedSpinMutex.impl

class alignas(std::hardware_destructive_interference_size) SharedSpinMutex
{
public:
	SharedSpinMutex()
		: m_ticketActive{ true, false }
	{}

	VT_INLINE void Lock()
	{
		uint32_t writerNumber = m_writerCounter.fetch_add(1, std::memory_order::acq_rel);

		while (writerNumber != m_nextWriter.load(std::memory_order::acquire))
		{
			//VT_PAUSE_THREAD();
		}

		WaitOnReaders();
	}


	VT_INLINE void Unlock()
	{
		m_ticketActive[m_readerTicketIndex].store(true, std::memory_order::release);
		m_nextWriter.fetch_add(1, std::memory_order::acq_rel);
	}

	VT_INLINE bool TryLock()
	{
		uint32_t writerNumber = m_writerCounter.load(std::memory_order::acquire);
		if (writerNumber != m_nextWriter.load(std::memory_order::acquire))
		{
			return false;
		}

		if (!m_writerCounter.compare_exchange_weak(writerNumber, writerNumber + 1, std::memory_order::acq_rel, std::memory_order::relaxed))
		{
			return false;
		}

		WaitOnReaders();
		return true;
	}

	VT_INLINE void LockShared()
	{
		uint32_t readerTicketIndex;
		uint32_t currentWriter = m_currentWriter.load(std::memory_order::acquire);

		while (true)
		{
			readerTicketIndex = m_newReaderTicketIndex.load(std::memory_order::acquire);
			m_ticketReaderCount[readerTicketIndex].fetch_add(1, std::memory_order::acq_rel);

			uint32_t nextWriter = m_currentWriter.load(std::memory_order::acquire);
			if (nextWriter == currentWriter)
			{
				break;
			}

			m_ticketReaderCount[readerTicketIndex].fetch_sub(1, std::memory_order::acq_rel);
			currentWriter = nextWriter;
		}

		while (!m_ticketActive[readerTicketIndex].load(std::memory_order::acquire))
		{
			//VT_PAUSE_THREAD();
		}
	}

	VT_INLINE void UnlockShared()
	{
		m_ticketReaderCount[m_readerTicketIndex].fetch_sub(1, std::memory_order::acq_rel);
	}

	VT_INLINE bool TryLockShared()
	{
		uint32_t currentWriter = m_currentWriter.load(std::memory_order::acquire);
		uint32_t readerTicketIndex = m_newReaderTicketIndex.load(std::memory_order::acquire);
		m_ticketReaderCount[readerTicketIndex].fetch_add(1, std::memory_order::acq_rel);

		uint32_t nextWriter = m_currentWriter.load(std::memory_order::acquire);
		if (nextWriter != currentWriter)
		{
			m_ticketReaderCount[readerTicketIndex].fetch_sub(1, std::memory_order::acq_rel);
			return false;
		}

		if (m_ticketActive[readerTicketIndex].load(std::memory_order::acquire))
		{
			return true;
		}

		m_ticketReaderCount[readerTicketIndex].fetch_sub(1, std::memory_order::acq_rel);
		return false;
	}

private:
	inline static constexpr size_t CacheLineAlignment = std::hardware_destructive_interference_size;

	void WaitOnReaders()
	{
		std::uint_fast8_t flushingReaderTicket = m_newReaderTicketIndex.fetch_xor(1, std::memory_order::acq_rel);

		m_currentWriter.fetch_add(1, std::memory_order::acq_rel);

		while (m_ticketReaderCount[flushingReaderTicket].load(std::memory_order::acquire) != 0)
		{
			//VT_PAUSE_THREAD();
		}

		m_ticketActive[flushingReaderTicket].store(false, std::memory_order::release);
		m_readerTicketIndex = !flushingReaderTicket;
	}

	// Reader members
	alignas(CacheLineAlignment) std::atomic<bool> m_ticketActive[2];
	alignas(CacheLineAlignment) std::atomic<uint32_t> m_ticketReaderCount[2] = { 0 };
	alignas(CacheLineAlignment) std::atomic<std::uint_fast8_t> m_newReaderTicketIndex = 0;

	// Writer members
	alignas(CacheLineAlignment) std::atomic<uint32_t> m_writerCounter = 0;
	alignas(CacheLineAlignment) std::atomic<uint32_t> m_currentWriter = 0;
	[[no_unique_address]] alignas(CacheLineAlignment) std::atomic<uint32_t> m_nextWriter = 0;

	alignas(CacheLineAlignment) std::uint_fast8_t m_readerTicketIndex = false;
};
