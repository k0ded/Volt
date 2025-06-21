#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/V2/JobAllocator2.h"
#include "JobSystem/V2/Job2.h"

#include <SubSystem/SubSystem.h>
#include <EventSystem/EventListener.h>

#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Allocators/LinearAllocator.h>

namespace Volt
{

	class VTJS_API JobSystem2 : public SubSystem, EventListener
	{
	public:
		JobSystem2();
		~JobSystem2();

		template<typename Func> static Job2* CreateJob(std::string_view jobName, Func&& func);
		template<typename Func> static Job2* CreateJob(std::string_view jobName, JobCounter* associatedCounter, Func&& func);

		static JobCounter* CreateCounter();
		static void DestroyCounter(JobCounter* counter);

		static void RunJob(Job2* job);
		static void RunJobs(std::span<Job2*> jobs);

		static void WaitForCounter(JobCounter* counter);
		static void WaitForAndDestroyCounter(JobCounter*& counter);

		VT_DECLARE_SUBSYSTEM("{74BD3121-6E60-4372-8100-A1D4BA37EF54}"_guid)
	
	private:
		struct JobWorker
		{
			std::thread thread;
			WorkQueue<Job2*, QueueThreadingPolicy::MPMC> workQueue;
		};
		 
		void Initialize() override;
		void Shutdown() override;

		void SpawnWorker(uint32_t workerId);

		Job2* TryGetJob(uint32_t workerId);
		JobWorker* AllocateWorker(uint32_t workerId);

		void FinishJob(Job2* jobPtr);

		inline static constexpr size_t NumMaxWorkers = 64;
		inline static constexpr size_t NumMaxJobsPerQueue = 8192;
		inline static JobSystem2* s_instance = nullptr;

		std::atomic<bool> m_isAlive;
		std::atomic<uint32_t> m_nextQueueToPush = 0;
		std::condition_variable m_wakeCondition;
		std::mutex m_wakeMutex;
		uint32_t m_numWorkers = 0;

		Vector<JobWorker*> m_workers;
		Map<std::thread::id, uint32_t> m_workerThreadIDToIndex;

		LinearAllocator<sizeof(JobWorker) * NumMaxWorkers> m_workerAllocator;

		JobAllocator2<Job2, NumMaxJobsPerQueue> m_jobAllocator;
		JobAllocator2<JobCounter, NumMaxJobsPerQueue> m_counterAllocator;
	};

	template<typename Func>
	Job2* JobSystem2::CreateJob(std::string_view jobName, Func&& func)
	{
		JobCounter* newCounter = s_instance->m_counterAllocator.Allocate();
		newCounter->Reset();
		return CreateJob(jobName, newCounter, std::move(func));
	}

	template<typename Func>
	Job2* JobSystem2::CreateJob(std::string_view jobName, JobCounter* associatedCounter, Func&& func)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(associatedCounter->IsActive());

		Job2* newJob = s_instance->m_jobAllocator.Allocate();
		associatedCounter->Increment();
		newJob->Create(jobName, associatedCounter, func);

		return newJob;
	}
}
