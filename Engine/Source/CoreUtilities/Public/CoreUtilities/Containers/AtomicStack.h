#pragma once

#include "CoreUtilities/Containers/Array.h"

#include <atomic>
#include <cstdint>

template<typename T, size_t N>
class AtomicStack
{
public:
	AtomicStack()
	{
		m_dataHead.store(Pack(InvalidIndex, 0));

		InitializeFreeList();
	}

	bool Push(const T& value)
	{
		uint32_t index;
		if (!AllocateNode(index))
		{
			return false;
		}

		m_stack[index].value = value;

		uint64_t oldHead = m_dataHead.load();
		while (true)
		{
			uint32_t oldIndex = UnpackIndex(oldHead);
			uint32_t oldVersion = UnpackVersion(oldHead);
			
			m_stack[index].next.store(oldIndex);

			uint64_t newHead = Pack(index, oldVersion + 1);
			if (m_dataHead.compare_exchange_weak(oldHead, newHead))
			{
				return true;
			}
		}

		return false;
	}

	bool Pop(T& outValue)
	{
		uint64_t oldHead = m_dataHead.load();
		while (true)
		{
			uint32_t index = UnpackIndex(oldHead);
			uint32_t version = UnpackVersion(oldHead);

			if (index == InvalidIndex)
			{
				return false;
			}

			uint32_t nextIndex = m_stack[index].next.load();
			uint64_t newHead = Pack(nextIndex, version + 1);

			if (m_dataHead.compare_exchange_weak(oldHead, newHead))
			{
				outValue = std::move(m_stack[index].value);
				FreeNode(index);
				return true;
			}
		}

		return false;
	}

private:
	struct Node
	{
		T value;
		std::atomic<uint32_t> next;
	};

	inline static constexpr size_t CacheLineAlignment = std::hardware_destructive_interference_size;
	inline static constexpr uint32_t InvalidIndex = std::numeric_limits<uint32_t>::max();

	VT_INLINE static uint64_t Pack(uint32_t index, uint32_t version)
	{
		return (uint64_t(version) << 32) | index;
	}

	VT_INLINE static uint32_t UnpackIndex(uint64_t tagged)
	{
		return uint32_t(tagged & 0xFFFFFFFF);
	}

	VT_INLINE static uint32_t UnpackVersion(uint64_t tagged)
	{ 
		return uint32_t(tagged >> 32);
	}

	void InitializeFreeList()
	{
		for (size_t i = 0; i < N; i++)
		{
			m_stack[i].next.store(static_cast<uint32_t>(i) + 1 < N ? i + 1 : InvalidIndex);
		}

		m_freeHead.store(Pack(0, 0));
	}

	bool AllocateNode(uint32_t& outIndex)
	{
		uint64_t oldHead = m_freeHead.load();
		while (true)
		{
			uint32_t index = UnpackIndex(oldHead);
			uint32_t version = UnpackVersion(oldHead);

			if (index == InvalidIndex)
			{
				return false;
			}

			uint32_t next = m_stack[index].next.load();
			uint64_t newHead = Pack(next, version + 1);

			if (m_freeHead.compare_exchange_weak(oldHead, newHead))
			{
				outIndex = index;
				return true;
			}
		}

		return false;
	}

	void FreeNode(uint32_t index)
	{
		uint64_t oldHead = m_freeHead.load();
		while (true)
		{
			uint32_t headIndex = UnpackIndex(oldHead);
			uint32_t version = UnpackVersion(oldHead);

			m_stack[index].next.store(headIndex);

			uint64_t newHead = Pack(index, version + 1);

			if (m_freeHead.compare_exchange_weak(oldHead, newHead))
			{
				return;
			}
		}
	}

	alignas(CacheLineAlignment) Array<Node, N> m_stack;
	alignas(CacheLineAlignment) std::atomic<uint64_t> m_dataHead;
	alignas(CacheLineAlignment) std::atomic<uint64_t> m_freeHead;
};
