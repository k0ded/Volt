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
		inline static constexpr bool IsInline = true;

		ForElementType() = default;

		ForElementType(const ForElementType& other) noexcept
		{
			memcpy_s(m_data, TotalSize, other.m_data, TotalSize);
		}

		ForElementType(ForElementType&& other) noexcept
		{
			memmove_s(m_data, TotalSize, other.m_data, TotalSize);
		}

		ForElementType& operator=(const ForElementType& other) noexcept
		{
			if (&other != this)
			{
				memcpy_s(m_data, TotalSize, other.m_data, TotalSize);
			}
			return *this;
		}

		ForElementType& operator=(ForElementType&& other) noexcept
		{
			if (&other != this)
			{
				memmove_s(m_data, TotalSize, other.m_data, TotalSize);
			}
			return *this;
		}

		void* Allocate(size_t size, size_t alignment) noexcept
		{
			VT_ENSURE(size <= TotalSize);
			return m_data;
		}

		void Free(void* pointer) noexcept
		{

		}

	private:
		inline static constexpr size_t TotalSize = sizeof(ValueType) * NumValues;

		uint8_t m_data[TotalSize];
	};
};

