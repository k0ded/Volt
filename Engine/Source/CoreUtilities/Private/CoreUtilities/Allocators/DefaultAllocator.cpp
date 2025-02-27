#include "cupch.h"
#include "CoreUtilities/Allocators/DefaultAllocator.h"

#include "CoreUtilities/Allocator.h"

void* DefaultAllocator::Allocate(size_t size, size_t alignment)
{
	return g_heapAllocator->Allocate(size, alignment);
}

void DefaultAllocator::Free(void* pointer, size_t alignment)
{
	g_heapAllocator->Free(pointer, alignment);
}
