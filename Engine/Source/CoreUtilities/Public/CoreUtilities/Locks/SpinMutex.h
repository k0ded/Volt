#pragma once

#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/Math/Math.h"

#include <atomic>
#include <new>
#include <chrono>
#include <thread>

class SpinMutex
{
public:
	VT_INLINE void Lock()
	{
		std::uintmax_t spinCount = 0;

		auto sleepDuration = FirstSpinSleepDuration.count();

		auto waitFunc = [&spinCount, &sleepDuration]() 
		{
			++spinCount;

			const bool shouldSleep = Math::ModuloByPowerOfTwo(spinCount, std::uintmax_t(1024)) == 0;
			if (!shouldSleep)
			{
				VT_PAUSE_THREAD();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::microseconds(sleepDuration));
				sleepDuration = std::min(sleepDuration << 1, MaxSpinSleepDuration.count());
			}
		};

		while (true)
		{
			while (m_flag.test(std::memory_order::relaxed))
			{
				waitFunc();
			}

			if (!m_flag.test_and_set(std::memory_order::acquire))
			{
				// Locked.
				return;
			}

			waitFunc();
		}
	}

	VT_INLINE void Unlock()
	{
		m_flag.clear(std::memory_order::release);
	}

	VT_INLINE bool TryUnlock()
	{
		return !m_flag.test(std::memory_order::relaxed) && !m_flag.test_and_set(std::memory_order::acquire);
	}

private:
	inline static constexpr size_t CacheLineAlignment = std::hardware_destructive_interference_size;
	inline static constexpr std::chrono::microseconds FirstSpinSleepDuration{ 128 };
	inline static constexpr std::chrono::microseconds MaxSpinSleepDuration{ 512 * 1024 };

	alignas(CacheLineAlignment) std::atomic_flag m_flag;
};
