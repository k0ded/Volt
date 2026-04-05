#pragma once

#include <atomic>
#include <new>

class SpinMutex
{
public:
	VT_INLINE void lock()
	{
		while (true)
		{
			while (m_flag.test(std::memory_order::relaxed))
			{}

			if (!m_flag.test_and_set(std::memory_order::acquire))
			{
				// Locked.
				return;
			}
		}
	}

	VT_INLINE void unlock()
	{
		m_flag.clear(std::memory_order::release);
	}

	VT_INLINE bool try_unlock()
	{
		return !m_flag.test(std::memory_order::relaxed) && !m_flag.test_and_set(std::memory_order::acquire);
	}

private:
	inline static constexpr size_t CacheLineAlignment = std::hardware_destructive_interference_size;

	alignas(CacheLineAlignment) std::atomic_flag m_flag;
};
