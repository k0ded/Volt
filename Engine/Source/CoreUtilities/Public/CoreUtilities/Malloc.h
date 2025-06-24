#pragma once

#include "CoreUtilities/Config.h"

namespace Memory
{
	extern VTCOREUTIL_API void* Malloc(const size_t size, const size_t alignment = 0);
	extern VTCOREUTIL_API void Free(void* ptr);
}
