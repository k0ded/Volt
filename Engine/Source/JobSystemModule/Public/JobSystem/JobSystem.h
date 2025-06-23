#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/JobAllocator.h"
#include "JobSystem/Job.h"

#include <SubSystem/SubSystem.h>
#include <EventSystem/EventListener.h>

#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Allocators/LinearAllocator.h>

namespace Volt
{
	class AppUpdateEvent;

	class VTJS_API JobSystem : public SubSystem, EventListener
	{
	public:
		JobSystem();
		~JobSystem();

		template<typename Func> static Job* CreateJob(std::string_view jobName, Func&& func);
		template<typename Func> static Job* CreateJob(std::string_view jobName, ExecutionPolicy executionPolicy, Func&& func);
		template<typename Func> static Job* CreateJob(std::string_view jobName, JobCounter* associatedCounter, Func&& func);
		template<typename Func> static Job* CreateJob(std::string_view jobName, ExecutionPolicy executionPolicy, JobCounter* associatedCounter, Func&& func);
		template<typename Func> static Job* CreateJobAsDependency(std::string_view jobName, Job* dependantJob, Func&& func);

		static JobCounter* CreateCounter();
		static void DestroyCounter(JobCounter* counter);

		static void RunJob(Job* job);
		static void RunJobs(std::span<Job*> jobs);

		static void WaitForCounter(JobCounter* counter);
		static void WaitForAndDestroyCounter(JobCounter*& counter);

		VT_DECLARE_SUBSYSTEM("{74BD3121-6E60-4372-8100-A1D4BA37EF54}"_guid)
	
	private:
		// Used to implement ref counting.
		friend class JobCounter;
		friend class Job;

		struct JobWorker
		{
			std::thread thread;
			WorkQueue<Job*, QueueThreadingPolicy::MPMC> workQueue;
		};
		 
		void Initialize() override;
		void Shutdown() override;

		bool OnUpdate(AppUpdateEvent& event);
		void ExecuteMainThreadJobs();

		void SpawnWorker(uint32_t workerId);

		Job* TryGetJob(uint32_t workerId);
		JobWorker* AllocateWorker(uint32_t workerId);

		void FinishJob(Job* jobPtr);

		JobCounter* AllocateCounter(bool initializeWithRef = true);
		Job* AllocateJob();
		void FreeCounter(JobCounter* counter);
		void FreeJob(Job *job);

		void PushToWaitingList(Job* job);
		bool FlushWaitingList();

		inline static constexpr size_t NumMaxWorkers = 64;
		inline static constexpr size_t NumMaxJobsPerQueue = 8192;
		inline static JobSystem* s_instance = nullptr;

		std::atomic<bool> m_isAlive;
		std::atomic<uint32_t> m_nextQueueToPush = 0;
		std::condition_variable m_wakeCondition;
		std::mutex m_wakeMutex;
		std::mutex m_waitingListMutex;
		uint32_t m_numWorkers = 0;

		Vector<JobWorker*> m_workers;
		WorkQueue<Job*, QueueThreadingPolicy::MPSC> m_mainThreadQueue;
		Map<std::thread::id, uint32_t> m_workerThreadIDToIndex;

		LinearAllocator<sizeof(JobWorker) * NumMaxWorkers> m_workerAllocator;

		JobAllocator<Job, NumMaxJobsPerQueue> m_jobAllocator;
		JobAllocator<JobCounter, NumMaxJobsPerQueue> m_counterAllocator;
		AtomicStack<Job*, NumMaxJobsPerQueue> m_waitingList;
	};

	template<typename Func>
	Job* JobSystem::CreateJob(std::string_view jobName, Func&& func)
	{
		return CreateJob(jobName, ExecutionPolicy::WorkerThread, std::move(func));
	}

	template<typename Func>
	Job* JobSystem::CreateJob(std::string_view jobName, JobCounter* associatedCounter, Func&& func)
	{
		return CreateJob(jobName, ExecutionPolicy::WorkerThread, associatedCounter, std::move(func));
	}

	template<typename Func>
	Job* JobSystem::CreateJob(std::string_view jobName, ExecutionPolicy executionPolicy, Func&& func)
	{
		// We skip adding a ref to the counter here because the
		// the it's ref will be added later.
		JobCounter* associatedCounter = s_instance->AllocateCounter(false);
		return CreateJob(jobName, executionPolicy, associatedCounter, std::move(func));
	}

	template<typename Func>
	Job* JobSystem::CreateJob(std::string_view jobName, ExecutionPolicy executionPolicy, JobCounter* associatedCounter, Func&& func)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(associatedCounter->IsActive());

		JobCounter* waitCounter = s_instance->AllocateCounter();

		VT_ENSURE(waitCounter != associatedCounter);

		Job* newJob = s_instance->AllocateJob();
		associatedCounter->Increment();
		associatedCounter->IncRef();

		newJob->Create(jobName, associatedCounter, waitCounter, executionPolicy, func);

		return newJob;
	}

	template<typename Func>
	Job* JobSystem::CreateJobAsDependency(std::string_view jobName, Job* dependantJob, Func&& func)
	{
		return CreateJob(jobName, dependantJob->GetWaitCounter(), std::move(func));
	}
}
