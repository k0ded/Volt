#pragma once

#include "JobSystem/JobFiber.h"

#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/Allocators/FixedSizeArenaAllocator.h>

namespace Volt
{
	class FiberPool
	{
	public:
		void Initialize(uint32_t numFibers);

		JobFiber* TryGetFiber();
		void FreeFiber(JobFiber* fiber);

	private:
		FixedSizeArenaAllocator<JobFiber> m_fiberAllocator;
		AtomicStack<JobFiber*> m_fiberStack;
	};
}
