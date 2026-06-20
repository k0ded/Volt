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
#include <CoreUtilities/ConsoleVariableRegistry.h>

namespace Volt
{
	static thread_local uint32_t g_workerId = 0xFFFFFFFF;
	static uint32_t g_mainThreadWorkerId = 0xFFFFFFFF;

	static ConsoleVariable<int32_t> g_jobSystemUseFibers(
		"JobSystem.UseFibers",
		1,
		ConsoleVariableFlags::ReadOnly,
		"Whether to use fibers to execute jobs."
	);

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
		
		if (!JobSystem::s_instance->TryEnqueueJobOnCounter(workerData.scratch.toQueueWaitCounter, job))
		{
			// The counter was completed, queue the job for execution.
			JobSystem::s_instance->EnqueueJob(job);
		}

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
		: m_useFibersForExecution(g_jobSystemUseFibers.GetValue())
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;
	
		RegisterListener<AppTickEvent>(VT_BIND_EVENT_FN(JobSystem::OnTick));

		m_mainThreadQueue.Allocate(NumMaxMainThreadJobs);
		m_yieldedJobsReadyToRun.Allocate(NumMaxWaitingJobs);

		// Only initialize fibers if we are going to use them.
		if (IsUsingFibers())
		{
			m_fiberPool.Initialize(NumFibers);
		}
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

		s_instance->RunJobInternal(job);
	}

	void JobSystem::RunJobs(ArrayView<Job*> jobs)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(s_instance);

		s_instance->RunJobsInternal(jobs);
	}

	void JobSystem::YieldFromJob()
	{
		VT_ASSERT(g_workerId != 0xFFFFFFFF);
		
		// Yielding is only supported for fibers.
		if (s_instance->IsUsingFibers())
		{
			JobWorker& workerData = *s_instance->m_workers[g_workerId];
			if (VT_CHECK(workerData.currentlyExecutingJob != nullptr))
			{
				Job* job = workerData.currentlyExecutingJob;

				Threads::SetFiberExecutionID(-1);

				// Swap back to the worker context
				FiberSwapContext(&job->m_assignedFiber->m_executionContext, &workerData.fiberContext, OnFiberSwitch_PushToQueue, job);
			}
		}
	}

	void JobSystem::WaitForJob(Job* job)
	{
		for (JobCounterRef counter : job->GetAssociatedCounters())
		{
			WaitForCounter(counter);
		}
	}

	void JobSystem::WaitForCounter(JobCounter* counter)
	{
		if (!counter)
		{
			return;
		}

		if (s_instance->IsUsingFibers())
		{
			s_instance->WaitForCounter_FiberMode(counter);
		}
		else
		{
			s_instance->WaitForCounter_ThreadMode(counter);
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

		// Initialize the latch with the number of workers.
		m_startWorkersLatch.InitializeWithValue(m_numWorkers);
	
		for (uint32_t i = 0; i < hardwareConcurrency; ++i)
		{
			JobWorker* worker = m_workers.emplace_back(AllocateWorker(i));
			if (IsUsingFibers())
			{
				worker->thread = std::thread(std::bind(&JobSystem::SpawnWorker_FiberMode, this, i));
			}
			else
			{
				worker->thread = std::thread(std::bind(&JobSystem::SpawnWorker_ThreadMode, this, i));
			}

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
	}

	void JobSystem::Shutdown()
	{
		m_isRunning = false;

		for (auto worker : m_workers)
		{
			worker->workItemsAvailable.release();

			if (worker->thread.joinable())
			{
				worker->thread.join();
			}

			m_workerAllocator.Free(worker);
		}
	}

	bool JobSystem::OnTick(AppTickEvent& event)
	{
		if (IsUsingFibers())
		{
			ExecuteMainThreadJobs_FiberMode();
		}
		else
		{
			ExecuteMainThreadJobs_ThreadMode();
		}
		return false;
	}

	void JobSystem::ExecuteMainThreadJobs_ThreadMode()
	{
		JobWorker& workerData = *m_workers[g_workerId];
		Job* jobPtr;
		while (m_mainThreadQueue.Pop(jobPtr))
		{
			workerData.currentlyExecutingJob = jobPtr;
			jobPtr->ExecuteInternal();

			FinishJob(/* no fiber */nullptr, jobPtr);
		}

		workerData.currentlyExecutingJob = nullptr;
	}

	void JobSystem::ExecuteMainThreadJobs_FiberMode()
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

	bool JobSystem::IsUsingFibers() const
	{
		return m_useFibersForExecution;
	}

	void JobSystem::SpawnWorker_ThreadMode(uint32_t workerId)
	{
		Threads::InitializeThreadConfig(true, false, false);

		g_workerId = workerId;
		JobWorker& workerData = *m_workers[workerId];

		// Wait here for all threads to be created.
		m_startWorkersLatch.ArriveAndWait();
		
		while (m_isRunning.load(std::memory_order::relaxed))
		{
			Job* job = TryGetJob(workerId);
			if (job)
			{
				workerData.currentlyExecutingJob = job;
				job->ExecuteInternal();

				FinishJob(/* no fiber */nullptr, job);
			}
			else
			{
				workerData.currentlyExecutingJob = nullptr;
				workerData.workItemsAvailable.acquire();
			}
		}
	}

	void JobSystem::SpawnWorker_FiberMode(uint32_t workerId)
	{
		Threads::InitializeThreadConfig(true, false, false);

		g_workerId = workerId;
		JobWorker& workerData = *m_workers[workerId];

		// Wait here for all threads to be created.
		m_startWorkersLatch.ArriveAndWait();

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
			if (!successfullyRanJob)
			{
				Job* job = TryGetYieldedJob();
				workerData.currentlyExecutingJob = job;
				successfullyRanJob = ExecuteJob(job);
			}

			// No job was able to run, let's wait for one.
			if (!successfullyRanJob)
			{
				workerData.currentlyExecutingJob = nullptr;
				workerData.workItemsAvailable.acquire();
			}
		}
	}

	JobSystem::JobWorker* JobSystem::AllocateWorker(uint32_t workerId)
	{
		JobWorker* worker = m_workerAllocator.Allocate();
		return worker;
	}

	void JobSystem::NotifyCounterReady(JobCounterRef counter)
	{
		VT_PROFILE_FUNCTION();
		JobRef list = counter->m_waiterHead.exchange(JobCounter::WaitingListClosed, std::memory_order::acq_rel);

		uint32_t numYieldedJobsEnqueued = 0;
		while (list != nullptr)
		{
			JobRef next = list->m_nextWaiter;
			list->m_nextWaiter = nullptr;

			if (list->GetAssignedFiber() != nullptr)
			{
				m_yieldedJobsReadyToRun.Emplace(list->GetPriority(), list);
				++numYieldedJobsEnqueued;
			}
			else
			{
				EnqueueJob(list);
			}

			list = next;
		}

		WakeWorkers(numYieldedJobsEnqueued);
	}

	void JobSystem::WakeWorkers(uint32_t numJobs)
	{
		for (uint32_t i = 0; i < std::min(numJobs, m_numWorkers); ++i)
		{
			m_workers[i]->workItemsAvailable.release();
		}
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

	void JobSystem::RunJobInternal(Job* job)
	{
		// Enqueue job if it's ready to run, otherwise push to waiting list.
		if (job->GetWaitCounter()->IsCompleted())
		{
			EnqueueJob(job);
		}
		else if (!TryEnqueueJobOnCounter(job->GetWaitCounter(), job))
		{
			// Counter was completed, just enqueue the job instead.
			EnqueueJob(job);
		}
	}

	void JobSystem::RunJobsInternal(ArrayView<Job*> jobs)
	{
		for (Job* job : jobs)
		{
			RunJobInternal(job);
		}
	}

	void JobSystem::WaitForCounter_ThreadMode(JobCounterRef counter)
	{
		if (!counter->IsCompleted())
		{
			counter->WaitForCounterLocking();
		}
	}

	void JobSystem::WaitForCounter_FiberMode(JobCounterRef counter)
	{
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

				Threads::SetFiberExecutionID(-1);

				Job* job = worker.currentlyExecutingJob;
				FiberSwapContext(&job->m_assignedFiber->m_executionContext, &worker.fiberContext, OnFiberSwitch_PushToWaitingList, job);
			}
			else
			{
				counter->WaitForCounterLocking();
			}
		}
	}

	void JobSystem::EnqueueJob(JobRef job)
	{
		if (job->GetExecutionPolicy() == ExecutionPolicy::WorkerThread)
		{
			const uint32_t nextQueueToPush = m_nextQueueToPush.fetch_add(1, std::memory_order::relaxed) % m_numWorkers;

			m_workers[nextQueueToPush]->workQueues.Emplace(job->GetPriority(), job);
			m_workers[nextQueueToPush]->workItemsAvailable.release();
		}
		else
		{
			m_mainThreadQueue.Emplace(job);
		}
	}

	bool JobSystem::TryEnqueueJobOnCounter(JobCounterRef counter, JobRef job)
	{
		while (true)
		{
			JobRef listHead = counter->m_waiterHead.load(std::memory_order::acquire);
			if (listHead == JobCounter::WaitingListClosed)
			{
				// Counter was completed.
				return false;
			}

			job->m_nextWaiter = listHead;
			if (counter->m_waiterHead.compare_exchange_weak(
				listHead, job,
				std::memory_order::release,
				std::memory_order::acquire))
			{
				// Successfully enqueued.
				return true;
			}
		}

		return false;
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
			Threads::SetFiberExecutionID(assignedFiber->GetID());
			assignedFiber->ContinueExecution();
			return true;
		}
		else
		{
			const bool hasFiber = m_fiberPool.TryGetFiber(assignedFiber);
			bool ranJob = false;

			if (hasFiber && assignedFiber)
			{
				Threads::SetFiberExecutionID(assignedFiber->GetID());
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

		ArrayView<JobCounterRef> associatedCounters = job->GetAssociatedCounters();
		JobCounterRef waitCounter = job->GetWaitCounter();
	
		for (JobCounterRef counter : associatedCounters)
		{
			int32_t oldCount = counter->Decrement();
			if (oldCount == 1)
			{
				// Mark as freed by setting value to < 0.
				counter->Decrement();
			}
			counter->DecRef();
		}

		waitCounter->DecRef();
		job->DecRef();

		// Only switch fiber if we are using fibers.
		if (fiber)
		{
			Threads::SetFiberExecutionID(-1);

			// Move back to the worker fiber
			FiberSwapContext(&fiber->m_executionContext, &m_workers[g_workerId]->fiberContext, OnFiberSwitch_FreeFiber, fiber);
		}
	}
}
