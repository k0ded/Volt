#include "jspch.h"
#include "JobSystem/JobSystem.h"

#include <Volt-Platforms/Platform.h>

#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Allocators/InlineAllocator.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(JobSystem, Minimal, PreEngine, 3);

    JobSystem::JobSystem()
    {
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;

		RegisterListener<AppTickEvent>(VT_BIND_EVENT_FN(JobSystem::OnTick));

		for (uint32_t i = 0; i < static_cast<uint32_t>(ExecutionPriority::Num); ++i)
		{
			m_waitingList[i].Allocate(NumMaxWaitingJobs);
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

		// Enqueue job if it's ready to run, otherwise push to waiting list.
		if (job->GetWaitCounter()->IsCompleted())
		{
			if (job->GetExecutionPolicy() == ExecutionPolicy::WorkerThread)
			{
				const uint32_t nextQueueToPush = s_instance->m_nextQueueToPush.fetch_add(1, std::memory_order::relaxed) % s_instance->m_numWorkers;
				s_instance->m_workers.at(nextQueueToPush)->workQueues.at(static_cast<size_t>(job->GetPriority())).Emplace(job);
				s_instance->m_wakeCondition.notify_all();
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

	void JobSystem::RunJobs(std::span<Job*> jobs)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(s_instance);

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
						s_instance->m_workers.at(worker)->workQueues.at(static_cast<size_t>(job->GetPriority())).Emplace(job);
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

		s_instance->m_wakeCondition.notify_all();
	}

	void JobSystem::WaitForCounter(JobCounter* counter)
	{
		VT_PROFILE_FUNCTION();

		if (!counter)
		{
			return;
		}

		uint32_t workerId = 0;
		if (s_instance->m_workerThreadIDToIndex.contains(std::this_thread::get_id()))
		{
			workerId = s_instance->m_workerThreadIDToIndex.at(std::this_thread::get_id());
		}

		while (!counter->IsCompleted())
		{
			Job* jobPtr = s_instance->TryGetJob(workerId);
			if (jobPtr)
			{
				{
					VT_PROFILE_SCOPE(jobPtr->GetName().data());
					jobPtr->Execute();
				}

				s_instance->FinishJob(jobPtr);
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

	void JobSystem::Initialize()
    {
		m_isAlive = true;
		const uint32_t hardwareConcurrency = PlatformMisc::GetNumberOfPhysicalCores();
		m_numWorkers = hardwareConcurrency;

		m_mainThreadQueue.Allocate(1024);
		m_workers.reserve(m_numWorkers);
		m_workerThreadIDToIndex.reserve(m_numWorkers);

		for (uint32_t i = 0; i < hardwareConcurrency; ++i)
		{
			JobWorker* worker = m_workers.emplace_back(AllocateWorker(i));
			worker->thread = std::thread(std::bind(&JobSystem::SpawnWorker, this, i));
			
			for (uint8_t priority = 0; priority < static_cast<uint8_t>(ExecutionPriority::Num); ++priority)
			{
				worker->workQueues.at(priority).Allocate(NumMaxJobsPerQueue);
			}

			PlatformThread::AssignThreadToCore(worker->thread.native_handle(), 1ull << i);
			PlatformThread::SetThreadPriority(worker->thread.native_handle(), ThreadPriority::High);

			std::string threadName = std::format("Volt::Worker {}", i);
			PlatformThread::SetThreadName(worker->thread.native_handle(), threadName);
		}

		// Notify all threads that they are allowed to run.
		m_wakeCondition.notify_all();
    }

	void JobSystem::Shutdown()
    {
		m_isAlive = false;
		m_wakeCondition.notify_all();

		for (auto worker : m_workers)
		{
			worker->thread.join();

			// We need to manually call the destructor
			// because the worker is allocated in a linear allocator.
			worker->~JobWorker();
		}
    }

	bool JobSystem::OnTick(AppTickEvent& event)
	{
		ExecuteMainThreadJobs();
		return false;
	}

	void JobSystem::ExecuteMainThreadJobs()
	{
		Job* jobPtr;
		while (m_mainThreadQueue.Pop(jobPtr))
		{
			{
				VT_PROFILE_SCOPE(jobPtr->GetName().data());
				jobPtr->Execute();
			}

			FinishJob(jobPtr);
		}
	}

	void JobSystem::SpawnWorker(uint32_t workerId)
	{
		m_workerThreadIDToIndex[std::this_thread::get_id()] = workerId;

		// Wait here for all threads to be created.
		{
			std::unique_lock<std::mutex> spawnLock(m_wakeMutex);
			m_wakeCondition.wait(spawnLock);
		}

		while (m_isAlive.load(std::memory_order::relaxed))
		{
			Job* jobPtr = TryGetJob(workerId);

			if (jobPtr)
			{
				{
					VT_PROFILE_SCOPE(jobPtr->GetName().data());
					jobPtr->Execute();
				}

				FinishJob(jobPtr);
			}
			else
			{
				std::unique_lock<std::mutex> lock(m_wakeMutex);
				m_wakeCondition.wait(lock);
			}
		}
	}

	Job* JobSystem::TryGetJob(uint32_t workerId)
	{
		auto& worker = m_workers.at(workerId);

		// Get jobs per priority first. Steal jobs of the highest priority before working on lower
		// priority jobs.
		auto tryGetJobOfPriority = [&worker, workerId, this](ExecutionPriority priority, Job*& outJob) 
		{
			const size_t priorityAsIndex = static_cast<size_t>(priority);

			if (!worker->workQueues.at(priorityAsIndex).Pop(outJob))
			{
				uint32_t nextQueue = (workerId + 1) % m_numWorkers;
				while (nextQueue != workerId)
				{
					auto& stealingQueue = m_workers.at(nextQueue)->workQueues.at(priorityAsIndex);
					if (stealingQueue.Pop(outJob))
					{
						break;
					}

					nextQueue = (nextQueue + 1) % m_numWorkers;
				}
			}

			// If there still was no job, we flush this priorities waiting list.
			if (!outJob)
			{
				FlushWaitingList(priority);
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

	JobSystem::JobWorker* JobSystem::AllocateWorker(uint32_t workerId)
	{
		constexpr size_t WorkerSize = sizeof(JobWorker);
		void* allocatedPtr = m_workerAllocator.Allocate(WorkerSize);
		JobWorker* worker = new(allocatedPtr) JobWorker();

		return worker;
	}

	void JobSystem::FinishJob(Job* jobPtr)
	{
		JobCounter* counter = jobPtr->GetCounter();
		JobCounter* waitCounter = jobPtr->GetWaitCounter();

		int32_t oldCount = counter->Decrement();
		if (oldCount == 1)
		{
			// Mark as freed by setting value to < 0.
			counter->Decrement();
		}

		counter->DecRef();
		waitCounter->DecRef();
		jobPtr->DecRef();
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

	void JobSystem::PushToWaitingList(ExecutionPriority priority, Job* job)
	{
		m_waitingList.at(static_cast<size_t>(priority)).Push(job);
	}

	bool JobSystem::FlushWaitingList(ExecutionPriority priority)
	{
		VT_PROFILE_FUNCTION();

		auto& waitingList = m_waitingList.at(static_cast<size_t>(priority));

		if (waitingList.Size() == 0)
		{
			return false;
		}

		// Use a lock here to make sure that only one thread flushes at a time.
		std::scoped_lock lock{ m_waitingListMutex };

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
}
