#pragma once

#include "CoreUtilities/Config.h"

namespace Memory
{
	// Required to be called by entry point
	extern VTCOREUTIL_API void Initialize();
	extern VTCOREUTIL_API void* Malloc(size_t size, size_t alignment = 0);
	extern VTCOREUTIL_API void* Realloc(void* original, size_t size, size_t alignment = 0);
	extern VTCOREUTIL_API void Free(void* ptr);
}
