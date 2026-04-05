#include "cupch.h"

#include "CoreUtilities/Allocators/GlobalMemoryStack.h"
#include "CoreUtilities/Profiling/Profiling.h"
#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/ThreadConfig.h"

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
	constexpr size_t MaxFiberCount = 256;

	static Vector<GlobalMemoryStack> instances(MaxFiberCount);
	static thread_local GlobalMemoryStack threadInstance;

	const ThreadConfig& threadConfig = Threads::GetThreadConfig();

	if (threadConfig.activeFiberId != -1)
	{
		VT_ASSERT(threadConfig.activeFiberId < MaxFiberCount);
		return instances[threadConfig.activeFiberId];
	}

	return threadInstance;
}

void GlobalMemoryStack::ResetStackPointerTo(uint64_t value)
{
	m_allocator.ResetStackPointerTo(value);
}
