#pragma once

#include "CoreUtilities/Config.h"

class VTCOREUTIL_API DefaultAllocator
{
public:
	static void* Allocate(size_t size, size_t alignment);
	static void Free(void* pointer, size_t alignment);

private:
	DefaultAllocator() = delete;
};
