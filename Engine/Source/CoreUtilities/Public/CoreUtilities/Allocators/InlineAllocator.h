#pragma once

#include "CoreUtilities/Config.h"

#include <cstdint>

template<typename ValueType, size_t NumValues>
class InlineAllocator
{
public:
	void* Allocate(size_t size, size_t alignment);
	void Free(void* pointer);

private:
	uint8_t m_data[sizeof(ValueType) * NumValues];
};

template<typename ValueType, size_t NumValues>
inline void* InlineAllocator<ValueType, NumValues>::Allocate(size_t size, size_t alignment)
{
	VT_ENSURE(size < sizeof(ValueType) * NumValues);
	return m_data;
}

template<typename ValueType, size_t NumValues>
inline void InlineAllocator<ValueType, NumValues>::Free(void* pointer)
{
}
