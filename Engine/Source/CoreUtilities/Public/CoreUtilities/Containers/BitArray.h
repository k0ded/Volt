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

private:
	inline static constexpr value_type ArraySize = Math::DivideRoundUp(NumMaxItems, sizeof(uint64_t));
	inline static constexpr index_type NumBitsPerValue = sizeof(value_type) * 8u; // Note: Assumes 8 bits per byte.

	Array<value_type, ArraySize> m_data;
};
