#pragma once

#include "CoreUtilities/VoltAssert.h"

#include <atomic>

class Latch
{
public:
	constexpr Latch() noexcept
	{}

	Latch(const Latch&) = delete;
	Latch& operator=(const Latch&) = delete;

	constexpr explicit Latch(ptrdiff_t expectedCount) noexcept
		: m_counter(expectedCount)
	{}

	void InitializeWithValue(ptrdiff_t expectedCount) noexcept
	{
		m_counter = expectedCount;
	}

	void CountDown(ptrdiff_t update = 1) noexcept
	{
		VT_ASSERT(update >= 0);

		const ptrdiff_t currentValue = m_counter.fetch_sub(update) - update;
		if (currentValue == 0)
		{
			m_counter.notify_all();
		}
		else
		{
			VT_ASSERT(currentValue >= 0);
		}
	}

	VT_NODISCARD bool TryWait() const noexcept
	{
		return m_counter.load(std::memory_order::acquire) == 0;
	}

	void Wait() const noexcept
	{
		while (true)
		{
			const ptrdiff_t currentValue = m_counter.load(std::memory_order::acquire);
			if (currentValue == 0)
			{
				return;
			}
			else
			{
				VT_ASSERT(currentValue > 0);
			}

			m_counter.wait(currentValue, std::memory_order::relaxed);
		}
	}

	void ArriveAndWait(ptrdiff_t update = 1) noexcept
	{
		VT_ASSERT(update >= 0);

		const ptrdiff_t currentValue = m_counter.fetch_sub(update) - update;
		if (currentValue == 0)
		{
			m_counter.notify_all();
		}
		else
		{
			VT_ASSERT(currentValue > 0);
			m_counter.wait(currentValue, std::memory_order::relaxed);
			Wait();
		}
	}

private:
	std::atomic<ptrdiff_t> m_counter;
};
