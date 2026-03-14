#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/JobAllocator.h"
#include "JobSystem/Job.h"
#include "JobSystem/FiberCommon.h"
#include "JobSystem/FiberContext.h"
#include "JobSystem/JobStackAllocator.h"
#include "JobSystem/FiberPool.h"
#include "JobSystem/JobPriorityQueue.h"

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <EventSystem/EventListener.h>

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>
#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/Containers/ArrayView.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	class AppTickEvent;

	class VTJS_API JobSystem : public SubSystem, EventListener
	{
	public:
		JobSystem();
		~JobSystem();

		template<typename Func> VT_NODISCARD static Job* CreateJob(std::string_view jobName, ExecutionPriority priority, Func&& func, FiberStackSize stackSize = FiberStackSize::KB16);
		template<typename Func> VT_NODISCARD static Job* CreateJob(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, Func&& func, FiberStackSize stackSize = FiberStackSize::KB16);
		template<typename Func> VT_NODISCARD static Job* CreateJob(std::string_view jobName, ExecutionPriority priority, JobCounter* associatedCounter, Func&& func, FiberStackSize stackSize = FiberStackSize::KB16);
		template<typename Func> VT_NODISCARD static Job* CreateJob(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, JobCounter* associatedCounter, Func&& func, FiberStackSize stackSize = FiberStackSize::KB16);
		template<typename Func> VT_NODISCARD static Job* CreateJob(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, JobCounter* associatedCounter, JobCounter* waitCounter, Func&& func, FiberStackSize stackSize = FiberStackSize::KB16);
		template<typename Func> VT_NODISCARD static Job* CreateJobAsDependency(std::string_view jobName, Job* dependantJob, Func&& func, FiberStackSize stackSize = FiberStackSize::KB16);
		template<typename Func> VT_NODISCARD static Job* CreateJobWithDependency(std::string_view jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, Job* dependencyJob, Func&& func, FiberStackSize stackSize = FiberStackSize::KB16);

		static JobCounter* CreateCounter();
		static void DestroyCounter(JobCounter*& counter);

		static void RunJob(Job* job);
		static void RunJobs(ArrayView<Job*> jobs);
		static void YieldFromJob();

		static void WaitForCounter(JobCounter* counter);
		static void WaitForAndDestroyCounter(JobCounter*& counter);

		static uint32_t GetWorkerId();

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{09920657-D40D-4BA0-8040-A1AB1F4D3E51}"_guid)

	private:
		// Used to implement ref counting.
		friend class JobCounter;
		friend class Job;
		friend class JobFiber;

		friend void ExecuteFiber(void* userdata);
		friend void OnFiberSwitch_PushToQueue(void* userdata);
		friend void OnFiberSwitch_PushToWaitingList(void* userdata);
		friend void OnFiberSwitch_FreeFiber(void* userdata);

		inline static constexpr size_t NumMaxJobsPerQueue = 4096;
		inline static constexpr size_t NumMaxWaitingJobs = 1024;
		inline static constexpr size_t NumMaxMainThreadJobs = 1024;
		inline static constexpr uint32_t NumFibers = 160;

		struct WorkerScratch
		{
			JobCounterRef toQueueWaitCounter = nullptr;
		};

		struct JobWorker
		{
			std::thread thread;
			FiberContext fiberContext;
			WorkerScratch scratch;
			Job* currentlyExecutingJob = nullptr;

			VT_PROFILE_DECLARE_MUTEX(std::mutex, wakeMutex);
			JobPriorityQueue<Job*, QueueThreadingPolicy::MPMC> workQueues;
		};

		struct WaitingListEntry
		{
			JobRef job;
			JobCounterRef waitCounter;
		};

		void Initialize() override;
		void Shutdown() override;

		void AllocateWaitingLists();

		void PushToWaitingList(ExecutionPriority priority, JobRef job, JobCounterRef waitCounter);
		bool FlushWaitingList(ExecutionPriority priority);

		bool OnTick(AppTickEvent& event);
		void ExecuteMainThreadJobs();

		///// Worker management /////
		void SpawnWorker(uint32_t workerId);
		JobWorker* AllocateWorker(uint32_t workerId);

		void SpawnWaitingListManager();
		void NotifyCounterReady();

		///// Job/Counter management /////
		JobCounter* AllocateCounter(bool initializeWithRef = true);
		Job* AllocateJob();
		void FreeCounter(JobCounter* counter);
		void FreeJob(Job* job);

		bool AllocateStack(FiberStackSize stackSize, FiberStack& outStack);
		void FreeStack(FiberStack stack);

		///// Worker functions /////
		Job* TryGetJob(uint32_t workerId);
		Job* TryGetYieldedJob();
		bool ExecuteJob(Job* job);
		void FinishJob(JobFiber* fiber, Job* job);

		inline static JobSystem* s_instance = nullptr;

		uint32_t m_numWorkers = 0;

		alignas(std::hardware_constructive_interference_size) std::atomic_bool m_isRunning = true;
		alignas(std::hardware_constructive_interference_size) std::atomic_uint32_t m_nextQueueToPush = 0;

		std::condition_variable_any m_workerWakeCondition;
		std::condition_variable_any m_waitingListManangerCondition;
		VT_PROFILE_DECLARE_MUTEX_NAMED(std::mutex, m_waitingListManagerMutex, "JobSystemWaitingListManagerMutex");

		InlineVector<JobWorker*, 16> m_workers;

		WorkQueue<Job*, QueueThreadingPolicy::MPSC> m_mainThreadQueue;
		JobPriorityQueue<Job*, QueueThreadingPolicy::MPMC> m_yieldedJobsReadyToRun;
		JobPriorityQueue<WaitingListEntry, QueueThreadingPolicy::MPSC> m_waitingLists;

		std::thread m_waitingListManagerThread;

		PagedAtomicArenaAllocator<JobWorker, 16> m_workerAllocator;
		JobAllocator<Job> m_jobAllocator;
		JobAllocator<JobCounter> m_counterAllocator;

		JobStackAllocator m_stackAllocator;
		FiberPool m_fiberPool;
	};
}

#include "JobSystem/JobSystem.inl"
