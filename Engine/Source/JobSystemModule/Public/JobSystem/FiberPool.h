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

		bool TryGetFiber(JobFiber*& outFiber);
		void FreeFiber(JobFiber* fiber);

		bool HasAvailableFiber() const;

	private:
		FixedSizeArenaAllocator<JobFiber> m_fiberAllocator;
		AtomicStack<JobFiber*> m_fiberStack;
	};
}
