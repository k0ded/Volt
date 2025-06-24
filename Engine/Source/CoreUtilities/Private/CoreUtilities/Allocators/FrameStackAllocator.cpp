#include "cupch.h"

#include "CoreUtilities/Allocators/FrameStackAllocator.h"
#include "CoreUtilities/VoltAssert.h"

FrameStackAllocator g_frameStackAllocator;

void* FrameStackAllocator::AllocateOnStack(size_t size, size_t alignment)
{
	m_numAllocations++;
	return m_linearAllocator.Allocate(size);
}

void FrameStackAllocator::FreeOnStack(void* ptr)
{
	m_numAllocations--;
}

void FrameStackAllocator::ClearStack()
{
	VT_ENSURE_MSG(m_numAllocations == 0, "All allocations should have been freed at this point!");
	m_linearAllocator.Reset();
}

FrameStackAllocator& FrameStackAllocator::Get()
{
	return g_frameStackAllocator;
}

void* FrameStackAllocator::Mark::Allocate(size_t size, size_t alignment)
{
	return g_frameStackAllocator.AllocateOnStack(size, alignment);
}

void FrameStackAllocator::Mark::Free(void* pointer)
{
	if (pointer)
	{
		return g_frameStackAllocator.FreeOnStack(pointer);
	}
}
