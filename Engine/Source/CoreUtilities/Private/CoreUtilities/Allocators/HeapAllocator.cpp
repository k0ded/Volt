#include "cupch.h"
#include "CoreUtilities/Allocators/HeapAllocator.h"
#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/Profiling/Profiling.h"

void* HeapAllocator::Allocate(size_t size, size_t alignment)
{
	void* result;
	if (alignment <= MIN_PLATFORM_ALIGNMENT)
	{
		result = malloc(size);
	}
	else
	{
#ifdef VT_PLATFORM_WINDOWS
		result = _aligned_malloc(size, alignment);
#else
		result = malloc(size);
#endif
	}

	VT_PROFILE_ALLOC(result, size);
	return result;
}

void HeapAllocator::Free(void* pointer, size_t alignment)
{
	VT_PROFILE_FREE(pointer);

	if (alignment <= MIN_PLATFORM_ALIGNMENT)
	{
		free(pointer);
	}
	else
	{
#ifdef VT_PLATFORM_WINDOWS
		_aligned_free(pointer);
#else
		free(pointer);
#endif
	}
}
