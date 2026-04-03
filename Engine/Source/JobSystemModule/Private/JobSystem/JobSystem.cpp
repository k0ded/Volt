#include "jspch.h"

#include "JobSystem/JobSystem.h"
#include "JobSystem/JobFiber.h"
#include "JobSystem/Asm/FiberContext.h"

#include <EventSystem/EventSystem.h>
#include <EventSystem/ApplicationEvents.h>

#include <PlatformsModule/Platform.h>

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/Core.h>
#include <CoreUtilities/ThreadConfig.h>

namespace Volt
{
	static thread_local uint32_t g_workerId = 0xFFFFFFFF;
	static uint32_t g_mainThreadWorkerId = 0xFFFFFFFF;

	VT_REGISTER_SUBSYSTEM(JobSystem, Minimal, PreEngine);

	void OnFiberSwitch_PushToQueue(void* userdata)
	{
		Job* job = reinterpret_cast<Job*>(userdata);
		
		if (job->GetExecutionPolicy() == ExecutionPolicy::MainThread)
		{
			JobSystem::s_instance->m_mainThreadQueue.Emplace(job);
		}
		else
		{
			// Put this job back on the queue
			JobSystem::JobWorker& workerData = *JobSystem::s_instance->m_workers[g_workerId];
			workerData.workQueues.Emplace(job->GetPriority(), job);
		}
		VT_PROFILE_FIBER_LEAVE();
	}

	void OnFiberSwitch_PushToWaitingList(void* userdata)
	{
		Job* job = reinterpret_cast<Job*>(userdata);
		
		JobSystem::JobWorker& workerData = *JobSystem::s_instance->m_workers[g_workerId];

		// Put the job on the waiting list
		JobSystem::s_instance->PushToWaitingList(job->GetPriority(), job, workerData.scratch.toQueueWaitCounter);
		workerData.scratch.toQueueWaitCounter = nullptr;

		VT_PROFILE_FIBER_LEAVE();
	}

	void OnFiberSwitch_FreeFiber(void* userdata)
	{
		JobFiber* fiber = reinterpret_cast<JobFiber*>(userdata);
		fiber->Free();
		JobSystem::s_instance->m_fiberPool.FreeFiber(fiber);
	
		VT_PROFILE_FIBER_LEAVE();
	}

	JobSystem::JobSystem()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;
	
		RegisterListener<AppTickEvent>(VT_BIND_EVENT_FN(JobSystem::OnTick));

		AllocateWaitingLists();
		m_mainThreadQueue.Allocate(NumMaxMainThreadJobs);

		m_fiberPool.Initialize(NumFibers);
	}

	JobSystem::~JobSystem()
	{
		s_instance = nullptr;
	}

	JobCounter* JobSystem::CreateCounter()
	{
		return s_instance->AllocateCounter();
	}

	void JobSystem::DestroyCounter(JobCounter*& counter)
	{
		if (!counter)
		{
			return;
		}

		counter->DecRef();
		counter = nullptr;
	}

	void JobSystem::RunJob(Job* job)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(s_instance);

		// Enqueue job if it's ready to run, otherwise push to waiting list.
		if (job->GetWaitCounter()->IsCompleted())
		{
			if (job->GetExecutionPolicy() == ExecutionPolicy::WorkerThread)
			{
				const uint32_t nextQueueToPush = s_instance->m_nextQueueToPush.fetch_add(1, std::memory_order::relaxed) % s_instance->m_numWorkers;
				s_instance->m_workers[nextQueueToPush]->workQueues.Emplace(job->GetPriority(), job);
				s_instance->m_workerWakeCondition.notify_all();
			}
			else
			{
				s_instance->m_mainThreadQueue.Emplace(job);
			}
		}
		else
		{
			s_instance->PushToWaitingList(job->GetPriority(), job, job->GetWaitCounter());
		}
	}

	void JobSystem::RunJobs(ArrayView<Job*> jobs)
	{
		VT_PROFILE_FUNCTION();

		const uint32_t numJobs = static_cast<uint32_t>(jobs.size());
		const uint32_t numJobsPerWorker = numJobs / s_instance->m_numWorkers;
		const uint32_t remainder = numJobs - numJobsPerWorker * s_instance->m_numWorkers;

		for (uint32_t worker = 0; worker < s_instance->m_numWorkers; ++worker)
		{
			const uint32_t numJobsOnWorker = numJobsPerWorker + (worker == (s_instance->m_numWorkers - 1) ? remainder : 0);

			for (uint32_t index = 0; index < numJobsOnWorker; ++index)
			{
				const uint32_t jobIndex = numJobsPerWorker * worker + index;
				Job* job = jobs[jobIndex];

				if (job->GetWaitCounter()->IsCompleted())
				{
					if (job->GetExecutionPolicy() == ExecutionPolicy::WorkerThread)
					{
						s_instance->m_workers[worker]->workQueues.Emplace(job->GetPriority(), job);
					}
					else
					{
						s_instance->m_mainThreadQueue.Emplace(job);
					}
				}
				else
				{
					s_instance->PushToWaitingList(job->GetPriority(), job, job->GetWaitCounter());
				}
			}
		}

		s_instance->m_workerWakeCondition.notify_all();
	}

	void JobSystem::YieldFromJob()
	{
		VT_ASSERT(g_workerId != 0xFFFFFFFF);
		
		JobWorker& workerData = *s_instance->m_workers[g_workerId];
		if (VT_CHECK(workerData.currentlyExecutingJob != nullptr))
		{
			Job* job = workerData.currentlyExecutingJob;
			
			// Swap back to the worker context
			FiberSwapContext(&job->m_assignedFiber->m_executionContext, &workerData.fiberContext, OnFiberSwitch_PushToQueue, job);
		}
	}

	void JobSystem::WaitForCounter(JobCounter* counter)
	{
		if (!counter)
		{
			return;
		}

		const bool isWorkerThread = g_workerId != 0xFFFFFFFF;
		const bool isMainThread = g_workerId == g_mainThreadWorkerId;

		if (!counter->IsCompleted())
		{
			// If we're in a worker thread let's switch tasks,
			// otherwise just wait.
			if (isWorkerThread && !isMainThread)
			{
				auto& worker = *s_instance->m_workers[g_workerId];
				worker.scratch.toQueueWaitCounter = counter;

				Job* job = worker.currentlyExecutingJob;
				FiberSwapContext(&job->m_assignedFiber->m_executionContext, &worker.fiberContext, OnFiberSwitch_PushToWaitingList, job);
			}
			else
			{
				counter->WaitForCounterLocking();
			}
		}
	}

	void JobSystem::WaitForAndDestroyCounter(JobCounter*& counter)
	{
		if (!counter)
		{
			return;
		}

		WaitForCounter(counter);
		DestroyCounter(counter);

		counter = nullptr;
	}

	uint32_t JobSystem::GetWorkerId()
	{
		return g_workerId;
	}

	void JobSystem::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<EventSystem>();
	}

	void JobSystem::Initialize()
	{
		const uint32_t hardwareConcurrency = PlatformMisc::GetNumberOfPhysicalCores() - 1;
		m_numWorkers = hardwareConcurrency;
	
		for (uint32_t i = 0; i < hardwareConcurrency; ++i)
		{
			JobWorker* worker = m_workers.emplace_back(AllocateWorker(i));
			worker->thread = std::thread(std::bind(&JobSystem::SpawnWorker, this, i));
			worker->workQueues.Allocate(NumMaxJobsPerQueue);

			PlatformThread::AssignThreadToCore(worker->thread.native_handle(), 1ull << i);
			PlatformThread::SetThreadPriority(worker->thread.native_handle(), ThreadPriority::High);

			String threadName = FormatString("Volt::Worker {}", i);
			PlatformThread::SetThreadName(worker->thread.native_handle(), threadName);
		}

		// Setup a main thread worker
		{
			m_workers.emplace_back(AllocateWorker(hardwareConcurrency));
			
			//PlatformThread::AssignThreadToCore(PlatformThread::GetMainThreadHandle(), 1ull << hardwareConcurrency);
			PlatformThread::SetThreadPriority(PlatformThread::GetMainThreadHandle(), ThreadPriority::High);
			g_workerId = hardwareConcurrency;
			g_mainThreadWorkerId = g_workerId;
		}

		// Waiting list handler
		{
			m_waitingListManagerThread = std::thread(std::bind(&JobSystem::SpawnWaitingListManager, this));

			PlatformThread::SetThreadName(m_waitingListManagerThread.native_handle(), "Volt::WaitingListManager");
			PlatformThread::SetThreadPriority(m_waitingListManagerThread.native_handle(), ThreadPriority::High);
		}

		// Notify all threads that they are allowed to run.
		m_workerWakeCondition.notify_all();
	}

	void JobSystem::Shutdown()
	{
		m_isRunning = false;
		m_workerWakeCondition.notify_all();
		m_waitingListManangerCondition.notify_all();

		for (auto worker : m_workers)
		{
			if (worker->thread.joinable())
			{
				worker->thread.join();
			}

			m_workerAllocator.Free(worker);
		}

		m_waitingListManagerThread.join();
	}

	void JobSystem::AllocateWaitingLists()
	{
		m_waitingLists.Allocate(NumMaxWaitingJobs);
		m_yieldedJobsReadyToRun.Allocate(NumMaxWaitingJobs);
	}

	void JobSystem::PushToWaitingList(ExecutionPriority priority, JobRef job, JobCounterRef waitCounter)
	{
		WaitingListEntry entry
		{
			.job = job,
			.waitCounter = waitCounter
		};

		m_waitingLists.Emplace(priority, entry);
	}

	bool JobSystem::FlushWaitingList(ExecutionPriority priority)
	{
		VT_PROFILE_FUNCTION();

		if (m_waitingLists.Size(priority) == 0)
		{
			return false;
		}

		GlobalMemoryStackMark Mark;
		GlobalMemoryStackVector<WaitingListEntry> nonReadyJobs;
		nonReadyJobs.reserve(m_waitingLists.Size(priority));

		bool anyJobRun = false;

		WaitingListEntry entry;
		while (m_waitingLists.Pop(priority, entry))
		{
			if (entry.waitCounter->IsCompleted())
			{
				m_yieldedJobsReadyToRun.Emplace(entry.job->GetPriority(), entry.job);
				anyJobRun |= true;
			}
			else
			{
				nonReadyJobs.emplace_back(entry);
			}
		}

		for (auto job : nonReadyJobs)
		{
			m_waitingLists.Emplace(job.job->GetPriority(), job);
		}

		return anyJobRun;
	}

	bool JobSystem::OnTick(AppTickEvent& event)
	{
		ExecuteMainThreadJobs();
		return false;
	}

	void JobSystem::ExecuteMainThreadJobs()
	{
		JobWorker& workerData = *m_workers[g_workerId];
		Job* jobPtr;

		FiberGetContext(&workerData.fiberContext);

		while (m_mainThreadQueue.Pop(jobPtr))
		{
			workerData.currentlyExecutingJob = jobPtr;
			JobFiber* assignedFiber = jobPtr->GetAssignedFiber();
			if (assignedFiber)
			{
				assignedFiber->ContinueExecution();
			}
			else
			{
				if (m_fiberPool.TryGetFiber(assignedFiber))
				{
					assignedFiber->ExecuteJob(jobPtr);
				}
				else
				{
					RunJob(jobPtr);
				}
			}
		}

		workerData.currentlyExecutingJob = nullptr;
	}

	void JobSystem::SpawnWorker(uint32_t workerId)
	{
		Threads::InitializeThreadConfig(true, false, false);

		g_workerId = workerId;
		JobWorker& workerData = *m_workers[workerId];

		// Wait here for all threads to be created.
		{
			std::unique_lock spawnLock(workerData.wakeMutex);
			VT_PROFILE_LOCK_MARK(workerData.wakeMutex);
			m_workerWakeCondition.wait(spawnLock);
		}

		while (m_isRunning.load(std::memory_order::relaxed))
		{
			FiberGetContext(&workerData.fiberContext);

			bool successfullyRanJob = false;

			// Fiber available, we can try to run a job the normal way.
			if (m_fiberPool.HasAvailableFiber())
			{
				Job* job = TryGetJob(workerId);
				workerData.currentlyExecutingJob = job;
				successfullyRanJob = ExecuteJob(job);
			}
			// No fibers available, try to run a previously yielded job
			else
			{
				Job* job = TryGetYieldedJob();
				workerData.currentlyExecutingJob = job;
				successfullyRanJob = ExecuteJob(job);
			}

			// No job was able to run, let's wait for one.
			if (!successfullyRanJob)
			{
				workerData.currentlyExecutingJob = nullptr;

				std::unique_lock lock(workerData.wakeMutex);
				VT_PROFILE_LOCK_MARK(workerData.wakeMutex);
				m_workerWakeCondition.wait(lock, [this, workerId]()
				{
					return !m_isRunning.load(std::memory_order::relaxed) ||
						HasWorkAvailable(workerId);
				});
			}
		}
	}

	JobSystem::JobWorker* JobSystem::AllocateWorker(uint32_t workerId)
	{
		JobWorker* worker = m_workerAllocator.Allocate();
		return worker;
	}

	void JobSystem::SpawnWaitingListManager()
	{
		while (m_isRunning.load(std::memory_order::relaxed))
		{
			VT_PROFILE_SCOPE("FlushWaitingList");

			// Loop through the priorities in reverse to make sure we start with the
			// highest priority.
			m_waitingListRequiresFlush.store(false, std::memory_order::relaxed);

			bool anyJobRun = false;
			for (int32_t i = static_cast<int32_t>(ExecutionPriority::Num) - 1; i >= 0; --i)
			{
				anyJobRun |= FlushWaitingList(static_cast<ExecutionPriority>(i));
			}

			if (anyJobRun)
			{
				m_workerWakeCondition.notify_all();
			}

			std::unique_lock lock(m_waitingListManagerMutex);
			VT_PROFILE_LOCK_MARK(m_waitingListManagerMutex);
			m_waitingListManangerCondition.wait(lock, [this]()
			{
				return !m_isRunning.load(std::memory_order::relaxed) || 
					m_waitingListRequiresFlush.load(std::memory_order::relaxed);
			});
		}
	}

	void JobSystem::NotifyCounterReady()
	{
		m_waitingListRequiresFlush.store(true, std::memory_order::relaxed);
		m_waitingListManangerCondition.notify_one();
	}

	bool JobSystem::HasWorkAvailable(uint32_t workerId)
	{
		VT_PROFILE_FUNCTION();

		JobWorker& workerData = *m_workers[workerId];

		bool anyQueueHasWork = false;

		for (uint32_t i = 0; i < static_cast<uint32_t>(ExecutionPriority::Num); ++i)
		{
			ExecutionPriority priority = static_cast<ExecutionPriority>(i);

			anyQueueHasWork |= m_yieldedJobsReadyToRun.Size(priority) > 0;
			anyQueueHasWork |= workerData.workQueues.Size(priority) > 0;
		}

		return anyQueueHasWork;
	}

	JobCounter* JobSystem::AllocateCounter(bool initializeWithRef)
	{
		JobCounter* counter = m_counterAllocator.Allocate();

		if (initializeWithRef)
		{
			counter->IncRef();
		}

		return counter;
	}

	Job* JobSystem::AllocateJob()
	{
		Job* job = m_jobAllocator.Allocate();
		job->IncRef();

		return job;
	}

	void JobSystem::FreeCounter(JobCounter* counter)
	{
		VT_ENSURE_MSG(counter->IsCompleted(), "Counter must be completed!");
		counter->Reset();
		m_counterAllocator.Free(counter);
	}
	
	void JobSystem::FreeJob(Job* job)
	{
		job->Reset();
		m_jobAllocator.Free(job);
	}

	bool JobSystem::AllocateStack(FiberStackSize stackSize, FiberStack& outStack)
	{
		return m_stackAllocator.TryGetStack(stackSize, outStack);
	}

	void JobSystem::FreeStack(FiberStack stack)
	{
		m_stackAllocator.FreeStack(stack);
	}

	Job* JobSystem::TryGetJob(uint32_t workerId)
	{
		VT_PROFILE_FUNCTION();

		auto& worker = m_workers[workerId];

		// Get jobs per priority first. Steal jobs of the highest priority before working on lower
		// priority jobs.
		auto tryGetJobOfPriority = [&worker, workerId, this](ExecutionPriority priority, Job*& outJob)
		{
			if (!worker->workQueues.Pop(priority, outJob))
			{
				uint32_t nextQueue = (workerId + 1) % m_numWorkers;
				while (nextQueue != workerId)
				{
					if (nextQueue != g_mainThreadWorkerId)
					{
						if (m_workers[nextQueue]->workQueues.Pop(priority, outJob))
						{
							break;
						}
					}

					nextQueue = (nextQueue + 1) % m_numWorkers;
				}
			}
		};

		Job* job = nullptr;

		// Loop through the priorities in reverse to make sure we start with the
		// highest priority.
		for (int32_t i = static_cast<int32_t>(ExecutionPriority::Num) - 1; i >= 0; --i)
		{
			// Try to get a ready job first.
			if (m_yieldedJobsReadyToRun.Pop(static_cast<ExecutionPriority>(i), job))
			{
				break;
			}

			tryGetJobOfPriority(static_cast<ExecutionPriority>(i), job);
			if (job)
			{
				break;
			}
		}

		return job;
	}

	Job* JobSystem::TryGetYieldedJob()
	{
		Job* job = nullptr;
		// Loop through the priorities in reverse to make sure we start with the
		// highest priority.
		for (int32_t i = static_cast<int32_t>(ExecutionPriority::Num) - 1; i >= 0; --i)
		{
			// Try to get a ready job first.
			if (m_yieldedJobsReadyToRun.Pop(static_cast<ExecutionPriority>(i), job))
			{
				break;
			}
		}

		return job;
	}

	bool JobSystem::ExecuteJob(Job* job)
	{
		if (job == nullptr)
		{
			return false;
		}

		JobFiber* assignedFiber = job->GetAssignedFiber();
		if (assignedFiber)
		{
			assignedFiber->ContinueExecution();
			return true;
		}
		else
		{
			const bool hasFiber = m_fiberPool.TryGetFiber(assignedFiber);
			bool ranJob = false;

			if (hasFiber && assignedFiber)
			{
				ranJob = assignedFiber->ExecuteJob(job);
			}

			if (!hasFiber || !ranJob)
			{
				// No fiber available, requeue the job
				RunJob(job);

				// No stack space, release the fiber again
				m_fiberPool.FreeFiber(assignedFiber);

				return false;
			}
		}

		return true;
	}

	void JobSystem::FinishJob(JobFiber* fiber, Job* job)
	{
		VT_ENSURE(g_workerId != 0xFFFFFFFF);

		JobCounter* counter = job->GetCounter();
		JobCounter* waitCounter = job->GetWaitCounter();
	
		int32_t oldCount = counter->Decrement();
		if (oldCount == 1)
		{
			// Mark as freed by setting value to < 0.
			counter->Decrement();
		}

		counter->DecRef();
		waitCounter->DecRef();
		job->DecRef();

		// Move back to the worker fiber
		FiberSwapContext(&fiber->m_executionContext, &m_workers[g_workerId]->fiberContext, OnFiberSwitch_FreeFiber, fiber);
	}
}
