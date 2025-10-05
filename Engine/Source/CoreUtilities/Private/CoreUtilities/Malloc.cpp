#include "cupch.h"

#include "CoreUtilities/Malloc.h"
#include "CoreUtilities/Profiling/Profiling.h"

namespace Memory
{
#define USE_MIMALLOC 0

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

	void Initialize()
	{
#if USE_MIMALLOC
		InitializeMiMalloc();
#endif
	}

	void* Malloc(size_t size, size_t alignment)
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

	void* Realloc(void* original, size_t size, size_t alignment /*= 0*/)
	{
		constexpr size_t DefaultAlignment = 8;

		void* resultPtr = nullptr;

		if (alignment != DefaultAlignment)
		{
			alignment = std::max(size_t(size >= 16u ? 16u : 8u), alignment);
#if USE_MIMALLOC
			resultPtr = mi_realloc_aligned(original, size, alignment);
#else
			resultPtr = _aligned_malloc(size, alignment);
#endif
		}
		else
		{
			alignment = size_t(size >= 16u ? 16u : DefaultAlignment);
#if USE_MIMALLOC
			resultPtr = mi_realloc_aligned(original, size, alignment);
#else
			resultPtr = _aligned_malloc(size, alignment);
#endif
		}

#if !USE_MIMALLOC
		Free(original);
#endif

		VT_PROFILE_FREE(original);
		VT_PROFILE_ALLOC(resultPtr, size);
		return resultPtr;
	}

	void Free(void* ptr)
	{
		if (!ptr)
		{
			return;
		}

#if USE_MIMALLOC
		mi_free(ptr);
#else
		_aligned_free(ptr);
#endif

		VT_PROFILE_FREE(ptr);
	}
}
