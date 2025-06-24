#pragma once

#include "CoreUtilities/Config.h"

class VTCOREUTIL_API HeapAllocator
{
public:
	inline static constexpr bool IsInline = false;

	HeapAllocator();
	void* Allocate(size_t size, size_t alignment);
	void Free(void* pointer);
};
