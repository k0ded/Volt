#include "jspch.h"

#include "JobSystem/Job.h"
#include "JobSystem/JobSystem.h"
#include "JobSystem/Asm/FiberContext.h"

namespace Volt
{
	void JobCounter::DecRef()
	{
		const int32_t oldCount = m_referenceCount.fetch_sub(1, std::memory_order::release);
		VT_ASSERT(oldCount > 0);

		if (oldCount == 1)
		{
			std::atomic_thread_fence(std::memory_order::acquire);
			JobSystem::s_instance->FreeCounter(this);
		}
	}

	void JobCounter::NotifyCounterReady()
	{
		JobSystem::s_instance->NotifyCounterReady();
	}

	void Job::Reset()
	{
		m_allocated = false;
		m_waitCounter = nullptr;
		m_associatedCounters.set_capacity(0);
	}

	void Job::DecRef()
	{
		const int32_t oldCount = m_referenceCount.fetch_sub(1, std::memory_order::release);
		VT_ASSERT(oldCount > 0);

		if (oldCount == 1)
		{
			std::atomic_thread_fence(std::memory_order::acquire);
			JobSystem::s_instance->FreeJob(this);
		}
	}

	void Job::ExecuteInternal()
	{
		Job::JobFuncBase* funcPtr = GetJobFunction();
	
		{
			VT_PROFILE_SCOPE(m_jobName.data());
			funcPtr->Execute();
		}

		// Destroy the function.
		funcPtr->~JobFuncBase();
	}

	void Job::AddAssociatedCounter(JobCounterRef counter)
	{
		counter->Increment();
		counter->IncRef();

		m_associatedCounters.emplace_back(counter);
	}

	void Job::SetWaitCounter(JobCounterRef counter)
	{
		m_waitCounter = counter;
	}
}
