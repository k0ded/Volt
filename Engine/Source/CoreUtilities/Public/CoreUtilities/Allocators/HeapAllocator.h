#pragma once

#include "CoreUtilities/Config.h"

#include <unordered_set>

class VTCOREUTIL_API HeapAllocator
{
public:
	HeapAllocator();
	void* Allocate(size_t size, size_t alignment);
	void Free(void* pointer);
};
