#pragma once

#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	template<typename JobType, size_t NumMaxJobs>
	class JobAllocator2
	{
	public:
		JobAllocator2();

		JobType* Allocate();
		void Free(JobType* job);

	private:
		std::atomic<uint32_t> m_numAllocatedJobs;
		Array<JobType, NumMaxJobs> m_jobAllocator;

		AtomicStack<uint32_t, NumMaxJobs> m_availableJobStack;
	};

	template<typename JobType, size_t NumMaxJobs>
	void JobAllocator2<JobType, NumMaxJobs>::Free(JobType* job)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE_MSG(job >= &m_jobAllocator[0] && job < &m_jobAllocator[NumMaxJobs - 1], "Job does not belong to allocator!");
		
		const uint32_t jobIndex = static_cast<uint32_t>(std::distance(m_jobAllocator.begin(), job));
		m_availableJobStack.Push(jobIndex);
	}

	template<typename JobType, size_t NumMaxJobs>
	JobType* JobAllocator2<JobType, NumMaxJobs>::Allocate()
	{
		VT_PROFILE_FUNCTION();
		uint32_t jobIndex;

		// Try to get a job from the available job stack.
		if (!m_availableJobStack.Pop(jobIndex))
		{
			// Otherwise get a new one.
			jobIndex = m_numAllocatedJobs.fetch_add(1, std::memory_order::relaxed);
		}
	
		return &m_jobAllocator[jobIndex];
	}

	template<typename JobType, size_t NumMaxJobs>
	JobAllocator2<JobType, NumMaxJobs>::JobAllocator2()
		: m_numAllocatedJobs(0)
	{}
}
