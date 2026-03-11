#include "jspch.h"

#include "JobSystem/FiberPool.h"

namespace Volt
{
	void FiberPool::Initialize(uint32_t numFibers)
	{
		m_fiberAllocator.Reserve(numFibers);
		m_fiberStack.Allocate(numFibers);

		for (uint32_t i = 0; i < numFibers; ++i)
		{
			JobFiber* fiber = m_fiberAllocator.Allocate(std::format("Fiber {}", i));
			m_fiberStack.Push(fiber);
		}
	}

	JobFiber* FiberPool::TryGetFiber()
	{
		JobFiber* result = nullptr;
		VT_MAYBE_UNUSED bool success = m_fiberStack.Pop(result);
		VT_ASSERT(success);

		return result;
	}

	void FiberPool::FreeFiber(JobFiber* fiber)
	{
		m_fiberStack.Push(fiber);
	}
}
