#include "cupch.h"
#include "CoreUtilities/Allocators/HeapAllocator.h"
#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/Profiling/Profiling.h"
#include "CoreUtilities/VoltAssert.h"

// We opt out of using mimalloc in the debug configuration, as it ends up being very slow.
#ifndef VT_DEBUG
#define USE_MIMALLOC 1
#else
#define USE_MIMALLOC 0
#endif

#if USE_MIMALLOC
#include <mimalloc.h>

static void InitializeMiMalloc()
{
	static bool initialized = false;
	if (!initialized)
	{
		mi_option_set(mi_option_reset_delay, 10000);
		initialized = true;
	}
}
#endif

HeapAllocator::HeapAllocator()
{
#if USE_MIMALLOC
	InitializeMiMalloc();
#endif
}

void* HeapAllocator::Allocate(size_t size, size_t alignment)
{
	constexpr size_t DefaultAlignment = 8;

	void* resultPtr = nullptr;

	if (alignment != DefaultAlignment)
	{
		alignment = std::max(size_t(size >= 16u ? 16u : 8u), alignment);
#if USE_MIMALLOC
		resultPtr = mi_malloc_aligned(size, alignment);
#else
		resultPtr = _aligned_malloc(size, alignment);
#endif
	}
	else
	{
		alignment = size_t(size >= 16u ? 16u : DefaultAlignment);
#if USE_MIMALLOC
		resultPtr = mi_malloc_aligned(size, alignment);
#else
		resultPtr = _aligned_malloc(size, alignment);
#endif
	}

	VT_PROFILE_ALLOC(resultPtr, size);
	return resultPtr;
}

void HeapAllocator::Free(void* pointer)
{
	if (!pointer)
	{
		return;
	}

#if USE_MIMALLOC
	mi_free(pointer);
#else
	_aligned_free(pointer);
#endif

	VT_PROFILE_FREE(pointer);
}
