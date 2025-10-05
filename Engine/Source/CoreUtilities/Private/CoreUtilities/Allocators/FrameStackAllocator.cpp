#include "cupch.h"

#include "CoreUtilities/Allocators/FrameStackAllocator.h"
#include "CoreUtilities/VoltAssert.h"

FrameStackAllocator g_frameStackAllocator;

FrameStackAllocator::FrameStackAllocator()
{
	m_linearAllocator.Reserve(FrameStackSize);
}

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
