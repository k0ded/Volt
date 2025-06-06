#pragma once

#include "CoreUtilities/Config.h"

#include <cstdint>

template<size_t NumValues>
class InlineAllocator
{
public:
	template<typename ValueType>
	class ForElementType
	{
	public:
		void* Allocate(size_t size, size_t alignment)
		{
			constexpr size_t TypeSize = sizeof(ValueType);
			constexpr size_t BlockSize = TypeSize * NumValues;

			uint8_t* dataPtr = &m_data[BlockSize * size_t(m_index)];
			m_index == 0 ? m_index = 1 : m_index = 0;

			return dataPtr;
		}

		void Free(void* pointer)
		{

		}

	private:
		// We allocate double the data to support reallocation.
		// Not the best solution.
		uint8_t m_data[sizeof(ValueType) * NumValues * 2];
		uint8_t m_index = 0;
	};
};

