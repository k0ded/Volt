#include "cupch.h"

#include "CoreUtilities/Allocators/GlobalMemoryStack.h"
#include "CoreUtilities/Profiling/Profiling.h"

void* GlobalMemoryStack::Allocate(uint64_t numBytes, uint64_t alignment)
{
	VT_PROFILE_FUNCTION();

	if (!VT_CHECK_MSG(m_markStackDepth != 0, "Must have active mark to call allocate!"))
	{
		return nullptr;
	}

	return m_allocator.Allocate(numBytes, alignment);
}

GlobalMemoryStack& GlobalMemoryStack::Get()
{
	static thread_local GlobalMemoryStack instance;
	return instance;
}

void GlobalMemoryStack::ResetStackPointerTo(uint64_t value)
{
	m_allocator.ResetStackPointerTo(value);
}
