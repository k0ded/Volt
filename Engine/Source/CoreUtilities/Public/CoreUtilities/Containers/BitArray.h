#pragma once

#include "CoreUtilities/Containers/Array.h"
#include "CoreUtilities/Math/Math.h"

#include <concepts>

template<size_t NumMaxItems, std::integral UnderlyingType = uint64_t>
class BitArray
{
public:
	typedef UnderlyingType value_type;
	typedef uint32_t index_type;

	BitArray() noexcept
	{
		memset(m_data.data(), 0, sizeof(value_type) * m_data.size());
	}

	void SetBit(index_type index, bool value)
	{
		const index_type wordIndex = index / NumBitsPerValue;
		const index_type bitIndex = index % NumBitsPerValue;

		const index_type bitmask = (index_type(1) << bitIndex);

		if (value)
		{
			m_data[wordIndex] |= bitmask;
		}
		else
		{
			m_data[wordIndex] &= (~bitmask);
		}
	}

	bool IsBitSet(index_type index) const
	{
		VT_ASSERT(index < NumMaxItems);

		const index_type wordIndex = index / NumBitsPerValue;
		const index_type bitIndex = index % NumBitsPerValue;

		return m_data[wordIndex] & (index_type(1) << bitIndex);
	}

	index_type Rank(index_type bitIndex) const
	{
		index_type maxBitIndex = bitIndex % NumBitsPerValue;

		// Optimization for single element bit array
		if constexpr (NumMaxItems <= NumBitsPerValue)
		{
			return std::popcount(m_data[0] & ((index_type(1) << maxBitIndex) - index_type(1)));
		}
		else
		{
			index_type maxWordIndex = bitIndex / NumBitsPerValue;
			index_type numTotalBits = 0;

			for (size_t i = 0; i <= maxWordIndex; ++i)
			{
				numTotalBits += std::popcount(i == maxWordIndex ? (m_data[i] & ((index_type(1) << maxBitIndex) - index_type(1))) : m_data[i]);
			}

			return numTotalBits;
		}
	}

private:
	inline static constexpr index_type NumBitsPerValue = std::numeric_limits<value_type>::digits;
	inline static constexpr value_type ArraySize = Math::DivideRoundUp(NumMaxItems, size_t(NumBitsPerValue));

	Array<value_type, ArraySize> m_data;
};
