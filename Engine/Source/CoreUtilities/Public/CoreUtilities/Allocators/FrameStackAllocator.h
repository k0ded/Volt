#pragma once

#include "CoreUtilities/Config.h"

#include "CoreUtilities/Allocators/LinearAllocator.h"

class VTCOREUTIL_API FrameStackAllocator
{
public:
	FrameStackAllocator();

	// The actual allocator
	struct VTCOREUTIL_API Mark
	{
		inline static constexpr bool IsInline = false;

		void* Allocate(size_t size, size_t alignment);
		void Free(void* pointer);
	};

	void* AllocateOnStack(size_t size, size_t alignment);
	void FreeOnStack(void* ptr);
	void ClearStack();

	static FrameStackAllocator& Get();

private:
	inline static constexpr size_t FrameStackSize = 128 * 1024 * 1024; // 32MB

	LinearAllocator<DefaultHeapAllocator> m_linearAllocator;
	std::atomic_size_t m_numAllocations = 0;
};
