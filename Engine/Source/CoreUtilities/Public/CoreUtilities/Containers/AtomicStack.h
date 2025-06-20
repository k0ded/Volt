#pragma once

#include "CoreUtilities/Containers/Array.h"

#include <atomic>
#include <cstdint>

template<typename T, size_t N>
class AtomicStack
{
public:
	AtomicStack()
		: m_top(-1)
	{}

	bool Push(const T& value)
	{
		int32_t oldTop = m_top.load(std::memory_order::relaxed);
		while (oldTop + 1 < static_cast<int32_t>(N))
		{
			int32_t newTop = oldTop + 1;
			if (m_top.compare_exchange_weak(oldTop, newTop,
				std::memory_order::release, std::memory_order::relaxed))
			{
				m_stack[newTop] = value;
				return true;
			}
		}

		return false;
	}

	bool Pop(T& outValue)
	{
		int32_t oldTop = m_top.load(std::memory_order::relaxed);
		while (oldTop >= 0)
		{
			int32_t newTop = oldTop - 1;
			if (m_top.compare_exchange_weak(oldTop, newTop,
				std::memory_order::acquire, std::memory_order::relaxed))
			{
				outValue = std::move(m_stack[oldTop]);
				return true;
			}
		}

		return false;
	}

	size_t Size() const
	{
		return static_cast<size_t>(m_top.load(std::memory_order::relaxed) + 1);
	}

private:
	Array<T, N> m_stack;
	std::atomic<int32_t> m_top;
};
