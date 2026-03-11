#include "jspch.h"

#include "JobSystem/JobSystem.h"
#include "JobSystem/JobFiber.h"
#include "JobSystem/Asm/FiberContext.h"

#include <Volt-Platforms/Platform.h>

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/Core.h>

#include <EventSystem/EventSystem.h>
#include <EventSystem/ApplicationEvents.h>

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
			workerData.workQueues[std::to_underlying(job->GetPriority())].Emplace(job);
		}
	}

	void OnFiberSwitch_PushToWaitingList(void* userdata)
	{
		Job* job = reinterpret_cast<Job*>(userdata);
		
		// Put the job on the waiting list
		JobSystem::s_instance->PushToWaitingList(job->GetPriority(), job);
	}

	void OnFiberSwitch_FreeFiber(void* userdata)
	{
		JobFiber* fiber = reinterpret_cast<JobFiber*>(userdata);
		fiber->Free();
		JobSystem::s_instance->m_fiberPool.FreeFiber(fiber);
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
				s_instance->m_workers[nextQueueToPush]->workQueues[std::to_underlying(job->GetPriority())].Emplace(job);
				s_instance->m_workerWakeCondition.notify_all();
			}
			else
			{
				s_instance->m_mainThreadQueue.Emplace(job);
			}
		}
		else
		{
			s_instance->PushToWaitingList(job->GetPriority(), job);
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
						s_instance->m_workers[worker]->workQueues[std::to_underlying(job->GetPriority())].Emplace(job);
					}
					else
					{
						s_instance->m_mainThreadQueue.Emplace(job);
					}
				}
				else
				{
					s_instance->PushToWaitingList(job->GetPriority(), job);
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

			for (uint8_t priority = 0; priority < std::to_underlying(ExecutionPriority::Num); ++priority)
			{
				worker->workQueues[priority].Allocate(NumMaxJobsPerQueue);
			}

			PlatformThread::AssignThreadToCore(worker->thread.native_handle(), 1ull << i);
			PlatformThread::SetThreadPriority(worker->thread.native_handle(), ThreadPriority::High);

			std::string threadName = std::format("Volt::Worker {}", i);
			PlatformThread::SetThreadName(worker->thread.native_handle(), threadName);
		}

		// Setup a main thread worker
		{
			m_workers.emplace_back(AllocateWorker(hardwareConcurrency));
			
			PlatformThread::AssignThreadToCore(PlatformThread::GetMainThreadHandle(), 1ull << hardwareConcurrency);
			PlatformThread::SetThreadPriority(PlatformThread::GetMainThreadHandle(), ThreadPriority::High);
			g_workerId = hardwareConcurrency;
			g_mainThreadWorkerId = g_workerId;
		}

		// Notify all threads that they are allowed to run.
		m_workerWakeCondition.notify_all();
	}

	void JobSystem::Shutdown()
	{
		m_isRunning = false;
		m_workerWakeCondition.notify_all();

		for (auto worker : m_workers)
		{
			if (worker->thread.joinable())
			{
				worker->thread.join();
			}

			m_workerAllocator.Free(worker);
		}
	}

	void JobSystem::AllocateWaitingLists()
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(ExecutionPriority::Num); ++i)
		{
			m_waitingList[i].Allocate(NumMaxWaitingJobs);
		}
	}

	void JobSystem::PushToWaitingList(ExecutionPriority priority, Job* job)
	{
		m_waitingList[std::to_underlying(priority)].Push(job);
	}

	bool JobSystem::FlushWaitingList(ExecutionPriority priority)
	{
		VT_PROFILE_FUNCTION();

		auto& waitingList = m_waitingList[std::to_underlying(priority)];

		if (waitingList.Size() == 0)
		{
			return false;
		}

		// Use a lock here to make sure that only one thread flushes at a time.
		std::scoped_lock lock{ m_waitingListMutex };
		VT_PROFILE_LOCK_MARK(m_waitingListMutex);

		Vector<Job*, InlineAllocator<128>> nonReadyJobs;

		bool anyJobRun = false;

		Job* jobPtr;
		while (waitingList.Pop(jobPtr))
		{
			if (jobPtr->GetWaitCounter()->IsCompleted())
			{
				RunJob(jobPtr);
				anyJobRun |= true;
			}
			else
			{
				nonReadyJobs.emplace_back(jobPtr);
			}
		}

		for (auto job : nonReadyJobs)
		{
			waitingList.Push(job);
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

		if (workerData.currentlyExecutingJob)
		{
			VT_PROFILE_FIBER_LEAVE();
		}

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
				JobFiber* fiber = m_fiberPool.TryGetFiber();
				fiber->ExecuteJob(jobPtr);
			}
		}

		workerData.currentlyExecutingJob = nullptr;
	}

	void JobSystem::SpawnWorker(uint32_t workerId)
	{
		g_workerId = workerId;

		// Wait here for all threads to be created.
		{
			std::unique_lock spawnLock(m_wakeMutex);
			VT_PROFILE_LOCK_MARK(m_wakeMutex);
			m_workerWakeCondition.wait(spawnLock);
		}

		JobWorker& workerData = *m_workers[workerId];

		while (m_isRunning.load(std::memory_order::relaxed))
		{
			FiberGetContext(&workerData.fiberContext);

			Job* jobPtr = TryGetJob(workerId);

			if (workerData.currentlyExecutingJob)
			{
				VT_PROFILE_FIBER_LEAVE();
			}

			if (jobPtr)
			{
				workerData.currentlyExecutingJob = jobPtr;

				JobFiber* assignedFiber = jobPtr->GetAssignedFiber();
				if (assignedFiber)
				{
					assignedFiber->ContinueExecution();
				}
				else
				{
					JobFiber* fiber = m_fiberPool.TryGetFiber();
					fiber->ExecuteJob(jobPtr);
				}
			}
			else
			{
				workerData.currentlyExecutingJob = nullptr;

				std::unique_lock lock(m_wakeMutex);
				VT_PROFILE_LOCK_MARK(m_wakeMutex);
				m_workerWakeCondition.wait(lock);
			}
		}
	}

	JobSystem::JobWorker* JobSystem::AllocateWorker(uint32_t workerId)
	{
		JobWorker* worker = m_workerAllocator.Allocate();
		return worker;
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

	FiberStack JobSystem::AllocateStack(FiberStackSize stackSize)
	{
		if (stackSize == FiberStackSize::Small)
		{
			return m_stackAllocator.TryGetSmallStack();
		}
		else if (stackSize == FiberStackSize::Medium)
		{
			return m_stackAllocator.TryGetMediumStack();
		}
		else if (stackSize == FiberStackSize::Large)
		{
			return m_stackAllocator.TryGetLargeStack();
		}

		VT_ENSURE_NO_ENTRY();
		return{};
	}

	void JobSystem::FreeStack(FiberStack stack)
	{
		if (stack.GetStackSize() == FiberStackSize::Small)
		{
			m_stackAllocator.FreeSmallStack(stack);
		}
		else if (stack.GetStackSize() == FiberStackSize::Medium)
		{
			m_stackAllocator.FreeMediumStack(stack);
		}
		else if (stack.GetStackSize() == FiberStackSize::Medium)
		{
			m_stackAllocator.FreeLargeStack(stack);
		}
		else
		{
			VT_ENSURE_NO_ENTRY();
		}
	}

	Job* JobSystem::TryGetJob(uint32_t workerId)
	{
		VT_PROFILE_FUNCTION();

		auto& worker = m_workers[workerId];

		// Get jobs per priority first. Steal jobs of the highest priority before working on lower
		// priority jobs.
		auto tryGetJobOfPriority = [&worker, workerId, this](ExecutionPriority priority, Job*& outJob)
		{
			const size_t priorityAsIndex = static_cast<size_t>(priority);

			if (!worker->workQueues[priorityAsIndex].Pop(outJob))
			{
				uint32_t nextQueue = (workerId + 1) % m_numWorkers;
				while (nextQueue != workerId)
				{
					if (nextQueue != g_mainThreadWorkerId)
					{
						auto& stealingQueue = m_workers[nextQueue]->workQueues[priorityAsIndex];
						if (stealingQueue.Pop(outJob))
						{
							break;
						}
					}

					nextQueue = (nextQueue + 1) % m_numWorkers;
				}
			}

			// If there still was no job, we flush this priorities waiting list.
			if (!outJob)
			{
				bool shouldWakeWorkers = FlushWaitingList(priority);
				if (shouldWakeWorkers)
				{
					m_workerWakeCondition.notify_all();
				}
			}
		};

		Job* job = nullptr;

		// Loop through the priorities in reverse to make sure we start with the
		// highest priority.
		for (int32_t i = static_cast<int32_t>(ExecutionPriority::Num) - 1; i >= 0; --i)
		{
			tryGetJobOfPriority(static_cast<ExecutionPriority>(i), job);
			if (job)
			{
				break;
			}
		}

		return job;
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
