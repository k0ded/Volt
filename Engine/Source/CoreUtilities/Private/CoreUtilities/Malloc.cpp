#include "cupch.h"

#include "CoreUtilities/Malloc.h"

#include "CoreUtilities/Allocators/HeapAllocator.h"

namespace Memory
{
	HeapAllocator g_allocator;

	void* Malloc(const size_t size, const size_t alignment)
	{
		return g_allocator.Allocate(size, alignment);
	}

	void Free(void* ptr)
	{
		g_allocator.Free(ptr);
	}
}
