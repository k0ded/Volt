#pragma once

#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Math/Math.h"

#include <atomic>

template<typename IndexType, typename AllocatorType = DefaultHeapAllocator>
class AtomicBitVector
{
public:
	// Note: Assumes 8 bits per byte.
	inline static constexpr size_t IndexTypeBitCount = sizeof(IndexType) * 8;

	AtomicBitVector()
		: m_size(0)
	{ }

	VT_NODISCARD VT_INLINE size_t Size() const { return m_size; }

	void Resize(const size_t numBits)
	{
		const size_t numBitmasks = Math::DivideRoundUp(numBits, IndexTypeBitCount);
		m_bitArray.resize(numBitmasks);
		m_size = numBits;
	}

	bool IsBitSet(const size_t index, std::memory_order memoryOrder)
 	{
		VT_ENSURE(index < m_size);

		const size_t wordIndex = index / IndexTypeBitCount;
		const size_t bitIndex = index % IndexTypeBitCount;

		const IndexType bitmask = m_bitArray.at(wordIndex).atomic.load(memoryOrder);

		return bitmask & (IndexType(1) << bitIndex);
	}

	void SetBit(const size_t index, bool value, std::memory_order memoryOrder)
	{
		VT_ENSURE(index < m_size);

		const size_t wordIndex = index / IndexTypeBitCount;
		const size_t bitIndex = index % IndexTypeBitCount;

		const IndexType bitmask = (IndexType(1) << bitIndex);

		if (value)
		{
			m_bitArray[wordIndex].atomic.fetch_or(bitmask, memoryOrder);
		}
		else
		{
			m_bitArray[wordIndex].atomic.fetch_and(~bitmask, memoryOrder);
		}
	}

private:
	template<typename T>
	struct AtomicWrapper
	{
		std::atomic<T> atomic;

		AtomicWrapper() = default;
		AtomicWrapper(const std::atomic<T>& other)
			: atomic(other.load())
		{ }

		AtomicWrapper(const AtomicWrapper& other)
			: atomic(other.atomic.load())
		{ }

		AtomicWrapper& operator=(const AtomicWrapper& other)
		{
			atomic.store(other.atomic.load());
		}
	};

	Vector<AtomicWrapper<IndexType>, AllocatorType> m_bitArray;
	size_t m_size;
};
