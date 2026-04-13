#include "cupch.h"

#include "CoreUtilities/Malloc.h"
#include "CoreUtilities/MemoryTracker.h"
#include "CoreUtilities/MemoryUtility.h"
#include "CoreUtilities/Profiling/Profiling.h"

#include <mimalloc.h>

namespace Memory
{
	static void InitializeMiMalloc()
	{
		static bool initialized = false;
		if (!initialized)
		{
			mi_option_set(mi_option_reset_delay, 10000);
			initialized = true;
		}
	}

	void Initialize()
	{
		InitializeMiMalloc();
	}

	void* Malloc(size_t size, size_t alignment)
	{
		VT_PROFILE_FUNCTION();

		constexpr size_t DefaultAlignment = 8;
		alignment = std::max(size_t(size >= 16u ? 16u : DefaultAlignment), alignment);
		
		uint64_t toAllocSize = size;

#ifdef VT_ENABLE_MEMORY_TRACKER
		toAllocSize += sizeof(MemoryTrackerHeader) + alignment;
#endif
		void* basePtr = mi_malloc_aligned(toAllocSize, alignment);
		uintptr_t userPointer = reinterpret_cast<uintptr_t>(basePtr);

#ifdef VT_ENABLE_MEMORY_TRACKER
		userPointer = Utility::Align(userPointer + sizeof(MemoryTrackerHeader), alignment);

		MemoryTrackerHeader* header = reinterpret_cast<MemoryTrackerHeader*>(userPointer - sizeof(MemoryTrackerHeader));
		header->basePtr = basePtr;
		header->alignment = alignment;
		header->size = size;

		MemoryTracker::OnAllocate(header);
#endif
		//VT_PROFILE_ALLOC(resultPtr, size);
		return reinterpret_cast<void*>(userPointer);
	}

	void* Realloc(void* original, size_t size, size_t alignment /*= 0*/)
	{
		constexpr size_t DefaultAlignment = 8;

		uint64_t toAllocSize = size;

		if (!original)
		{
			return Malloc(toAllocSize, alignment);
		}

#ifdef VT_ENABLE_MEMORY_TRACKER
		MemoryTrackerHeader* header = GetHeader(original);

		const uint64_t oldSize = header->size;
		alignment = header->alignment;
		original = header->basePtr;

		toAllocSize += sizeof(MemoryTrackerHeader) + alignment;
#endif

		if (size == 0)
		{
			mi_free(original);
			return nullptr;
		}

		void* resultPtr = nullptr;

		if (alignment != DefaultAlignment)
		{
			alignment = std::max(size_t(size >= 16u ? 16u : 8u), alignment);
			resultPtr = mi_realloc_aligned(original, toAllocSize, alignment);
		}
		else
		{
			resultPtr = mi_realloc(original, toAllocSize);
		}

#ifdef VT_ENABLE_MEMORY_TRACKER
		if (!resultPtr)
		{
			return nullptr;
		}

		uintptr_t userPointer = reinterpret_cast<uintptr_t>(resultPtr) + sizeof(MemoryTrackerHeader);
		userPointer = Utility::Align(userPointer, alignment);

		MemoryTrackerHeader* newHeader = reinterpret_cast<MemoryTrackerHeader*>(userPointer - sizeof(MemoryTrackerHeader));
		newHeader->alignment = alignment;
		newHeader->basePtr = resultPtr;
		newHeader->size = size;
		
		resultPtr = reinterpret_cast<void*>(userPointer);

		MemoryTracker::OnReallocate(newHeader, oldSize);
#endif

		//VT_PROFILE_FREE(original);
		//VT_PROFILE_ALLOC(resultPtr, size);
		return resultPtr;
	}

	void Free(void* ptr)
	{
		if (!ptr)
		{
			return;
		}

		//VT_PROFILE_FREE(ptr);

#ifdef VT_ENABLE_MEMORY_TRACKER
		MemoryTrackerHeader* header = GetHeader(ptr);
		MemoryTracker::OnFree(header);
		mi_free(header->basePtr);
#else
		mi_free(ptr);
#endif
	}

#ifdef VT_ENABLE_MEMORY_TRACKER
	MemoryTrackerHeader* GetHeader(void* ptr)
	{
		return reinterpret_cast<MemoryTrackerHeader*>(reinterpret_cast<uintptr_t>(ptr) - sizeof(MemoryTrackerHeader));
	}
#endif
}
