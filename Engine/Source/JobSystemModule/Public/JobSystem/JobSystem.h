#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/JobAllocator.h"
#include "JobSystem/Job.h"

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <EventSystem/EventListener.h>

#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Containers/Array.h>
#include <CoreUtilities/Allocators/FixedSizeLinearAllocator.h>

namespace Volt
{
	class AppTickEvent;

	class VTJS_API JobSystem : public SubSystem, EventListener
	{
	public:
		JobSystem();
		~JobSystem();

		template<typename Func> VT_NODISCARD static Job* CreateJob(std::string_view jobName, ExecutionPriority priority, Func&& func);
		template<typename Func> VT_NODISCARD static Job* CreateJob(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, Func&& func);
		template<typename Func> VT_NODISCARD static Job* CreateJob(std::string_view jobName, ExecutionPriority priority, JobCounter* associatedCounter, Func&& func);
		template<typename Func> VT_NODISCARD static Job* CreateJob(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, JobCounter* associatedCounter, Func&& func);
		template<typename Func> VT_NODISCARD static Job* CreateJob(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, JobCounter* associatedCounter, JobCounter* waitCounter, Func&& func);
		template<typename Func> VT_NODISCARD static Job* CreateJobAsDependency(std::string_view jobName, Job* dependantJob, Func&& func);
		template<typename Func> VT_NODISCARD static Job* CreateJobWithDependency(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, Job* dependencyJob, Func&& func);

		static JobCounter* CreateCounter();
		static void DestroyCounter(JobCounter*& counter);

		static void RunJob(Job* job);
		static void RunJobs(std::span<Job*> jobs);

		static void WaitForCounter(JobCounter* counter);
		static void WaitForAndDestroyCounter(JobCounter*& counter);

		static uint32_t GetNumWorkers();

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{74BD3121-6E60-4372-8100-A1D4BA37EF54}"_guid)
	
	private:
		// Used to implement ref counting.
		friend class JobCounter;
		friend class Job;

		struct JobWorker
		{
			std::thread thread;
			Array<WorkQueue<Job*, QueueThreadingPolicy::MPMC>, static_cast<size_t>(ExecutionPriority::Num)> workQueues;
		};
		 
		void Initialize() override;
		void Shutdown() override;

		bool OnTick(AppTickEvent& event);
		void ExecuteMainThreadJobs();

		void SpawnWorker(uint32_t workerId);

		Job* TryGetJob(uint32_t workerId);
		JobWorker* AllocateWorker(uint32_t workerId);

		void FinishJob(Job* jobPtr);

		JobCounter* AllocateCounter(bool initializeWithRef = true);
		Job* AllocateJob();
		void FreeCounter(JobCounter* counter);
		void FreeJob(Job *job);

		void PushToWaitingList(ExecutionPriority priority, Job* job);
		bool FlushWaitingList(ExecutionPriority priority);

		inline static constexpr size_t NumMaxWorkers = 32;
		inline static constexpr size_t NumMaxJobsPerQueue = 4096;
		inline static constexpr size_t NumMaxWaitingJobs = 1024;

		inline static JobSystem* s_instance = nullptr;

		std::atomic<bool> m_isAlive;
		std::atomic<uint32_t> m_nextQueueToPush = 0;
		std::condition_variable_any m_wakeCondition;

		VT_PROFILE_DECLARE_MUTEX(std::mutex, m_wakeMutex);
		VT_PROFILE_DECLARE_MUTEX(std::mutex, m_waitingListMutex);

		uint32_t m_numWorkers = 0;

		Vector<JobWorker*> m_workers;
		WorkQueue<Job*, QueueThreadingPolicy::MPSC> m_mainThreadQueue;
		Map<std::thread::id, uint32_t> m_workerThreadIDToIndex;

		FixedSizeLinearAllocator<DefaultHeapAllocator> m_workerAllocator;

		JobAllocator<Job> m_jobAllocator;
		JobAllocator<JobCounter> m_counterAllocator;
		Array<AtomicStack<Job*>, static_cast<size_t>(ExecutionPriority::Num)> m_waitingList;
	};

	template<typename Func>
	Job* JobSystem::CreateJob(std::string_view jobName, ExecutionPriority priority, Func&& func)
	{
		return CreateJob(jobName, priority, ExecutionPolicy::WorkerThread, std::move(func));
	}

	template<typename Func>
	Job* JobSystem::CreateJob(std::string_view jobName, ExecutionPriority priority, JobCounter* associatedCounter, Func&& func)
	{
		return CreateJob(jobName, priority, ExecutionPolicy::WorkerThread, associatedCounter, std::move(func));
	}

	template<typename Func>
	Job* JobSystem::CreateJob(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, Func&& func)
	{
		return CreateJob(jobName, priority, executionPolicy, nullptr, std::move(func));
	}

	template<typename Func>
	Job* JobSystem::CreateJob(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, JobCounter* associatedCounter, Func&& func)
	{
		// Will create a wait counter for us
		return CreateJob(jobName, priority, executionPolicy, associatedCounter, nullptr, std::move(func));
	}

	template<typename Func>
	Job* JobSystem::CreateJob(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, JobCounter* associatedCounter, JobCounter* waitCounter, Func&& func)
	{
		VT_PROFILE_FUNCTION();
		if (!associatedCounter)
		{
			associatedCounter = s_instance->AllocateCounter(false);
		}
		VT_ENSURE(associatedCounter->IsActive());

		if (!waitCounter)
		{
			waitCounter = s_instance->AllocateCounter();
		}
		else
		{
			waitCounter->IncRef();
		}

		VT_ENSURE(waitCounter != associatedCounter);

		Job* newJob = s_instance->AllocateJob();
		associatedCounter->Increment();
		associatedCounter->IncRef();

		newJob->Create(jobName, associatedCounter, waitCounter, priority, executionPolicy, func);

		return newJob;
	}

	template<typename Func>
	Job* JobSystem::CreateJobAsDependency(std::string_view jobName, Job* dependantJob, Func&& func)
	{
		// Inherit the priority.
		return CreateJob(jobName, dependantJob->GetPriority(), dependantJob->GetWaitCounter(), std::move(func));
	}

	template<typename Func>
	Job* JobSystem::CreateJobWithDependency(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, Job* dependencyJob, Func&& func)
	{
		return CreateJob(jobName, priority, executionPolicy, nullptr, dependencyJob->GetCounter(), std::move(func));
	}
}
