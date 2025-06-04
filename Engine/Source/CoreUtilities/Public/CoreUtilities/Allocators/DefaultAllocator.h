#pragma once

#include "CoreUtilities/Config.h"

#include "CoreUtilities/Allocators/HeapAllocator.h"

class VTCOREUTIL_API DefaultAllocator
{
public:
	static void* Allocate(size_t size, size_t alignment);
	static void Free(void* pointer, size_t alignment);

private:
	inline static HeapAllocator s_allocator;

	DefaultAllocator() = delete;
};
