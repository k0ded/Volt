#include "cupch.h"
#include "CoreUtilities/Allocators/DefaultAllocator.h"

void* DefaultAllocator::Allocate(size_t size, size_t alignment)
{
	return s_allocator.Allocate(size, alignment);
}

void DefaultAllocator::Free(void* pointer, size_t alignment)
{
	s_allocator.Free(pointer);
}
