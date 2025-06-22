#include "jspch.h"
#include "JobSystem/V2/Job2.h"
#include "JobSystem/V2/JobSystem2.h"

namespace Volt
{
	void JobCounter::DecRef()
	{
		const uint32_t oldCount = m_referenceCount.fetch_sub(1, std::memory_order::release);
		VT_ASSERT(oldCount > 0);

		if (oldCount == 1)
		{
			std::atomic_thread_fence(std::memory_order::release);
			JobSystem2::s_instance->FreeCounter(this);
		}
	}

	void Job2::Execute()
	{
		VT_ENSURE(m_allocated);

		if (m_allocated)
		{
			JobFuncBase* funcPtr = reinterpret_cast<JobFuncBase*>(&m_funcStorage);
			funcPtr->Execute();

			// Destroy the function.
			funcPtr->~JobFuncBase();
			m_allocated = false;
		}
	}

	void Job2::Reset()
	{
		m_allocated = false;
		m_counter = nullptr;
		m_waitCounter = nullptr;
	}

	void Job2::DecRef()
	{
		const uint32_t oldCount = m_referenceCount.fetch_sub(1, std::memory_order::release);
		VT_ASSERT(oldCount > 0);

		if (oldCount == 1)
		{
			std::atomic_thread_fence(std::memory_order::release);
			JobSystem2::s_instance->FreeJob(this);
		}
	}
}
