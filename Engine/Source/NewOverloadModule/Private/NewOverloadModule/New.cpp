#include <mimalloc.h>

#include <cstdint>
#include <new>

//void* operator new(size_t size)
//{
//	constexpr size_t DefaultAlignment = 8;
//	return mi_malloc_aligned(size, DefaultAlignment);
//}
//
//void operator delete(void* ptr) noexcept
//{
//	if (!ptr)
//	{
//		return;
//	}
//
//	mi_free(ptr);
//}
