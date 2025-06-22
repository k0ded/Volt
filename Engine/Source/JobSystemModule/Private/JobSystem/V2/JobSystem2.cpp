#include "jspch.h"
#include "JobSystem/V2/JobSystem2.h"

#include <Volt-Platforms/Platform.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(JobSystem2, PreEngine, 3);

    JobSystem2::JobSystem2()
    {
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;
    }

    JobSystem2::~JobSystem2()
    {
		s_instance = nullptr;
    }

	JobCounter* JobSystem2::CreateCounter()
	{
		return s_instance->AllocateCounter();
	}

	void JobSystem2::DestroyCounter(JobCounter* counter)
	{
		counter->DecRef();
	}

	void JobSystem2::RunJob(Job2* job)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(s_instance);

		// Enqueue job if it's ready to run, otherwise push to waiting list.
		if (job->GetWaitCounter()->IsCompleted())
		{
			const uint32_t nextQueueToPush = s_instance->m_nextQueueToPush.fetch_add(1, std::memory_order::relaxed) % s_instance->m_numWorkers;
			s_instance->m_workers.at(nextQueueToPush)->workQueue.Emplace(job);
			s_instance->m_wakeCondition.notify_all();
		}
		else
		{
			s_instance->PushToWaitingList(job);
		}
	}

	void JobSystem2::RunJobs(std::span<Job2*> jobs)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(s_instance);

		const uint32_t numJobs = static_cast<uint32_t>(jobs.size());
		const uint32_t numJobsPerWorker = numJobs / s_instance->m_numWorkers;
		const uint32_t remainder = numJobs - numJobsPerWorker * s_instance->m_numWorkers;

		for (uint32_t worker = 0; worker < s_instance->m_numWorkers; ++worker)
		{
			const uint32_t numJobsOnWorker = numJobsPerWorker + (worker == (s_instance->m_numWorkers - 1) ? remainder : 0);

			// #TODO_Ivar: Replace with multiple emplace in work queue.
			for (uint32_t index = 0; index < numJobsOnWorker; ++index)
			{
				const uint32_t jobIndex = numJobsPerWorker * worker + index;
				s_instance->m_workers.at(worker)->workQueue.Emplace(jobs[jobIndex]);
			}
		}

		s_instance->m_wakeCondition.notify_all();
	}

	void JobSystem2::WaitForCounter(JobCounter* counter)
	{
		VT_PROFILE_FUNCTION();

		uint32_t workerId = 0;
		if (s_instance->m_workerThreadIDToIndex.contains(std::this_thread::get_id()))
		{
			workerId = s_instance->m_workerThreadIDToIndex.at(std::this_thread::get_id());
		}

		while (!counter->IsCompleted())
		{
			Job2* jobPtr = s_instance->TryGetJob(workerId);
			if (jobPtr)
			{
				{
					VT_PROFILE_SCOPE(jobPtr->GetName().data());
					jobPtr->Execute();
				}

				s_instance->FinishJob(jobPtr);
			}
			else
			{
				s_instance->FlushWaitingList();
			}
		}
	}

	void JobSystem2::WaitForAndDestroyCounter(JobCounter*& counter)
	{
		WaitForCounter(counter);
		DestroyCounter(counter);

		counter = nullptr;
	}

	void JobSystem2::Initialize()
    {
		m_isAlive = true;
		const uint32_t hardwareConcurrency = PlatformMisc::GetNumberOfPhysicalCores();
		m_numWorkers = hardwareConcurrency;

		m_workers.reserve(m_numWorkers);
		m_workerThreadIDToIndex.reserve(m_numWorkers);

		for (uint32_t i = 0; i < hardwareConcurrency; ++i)
		{
			JobWorker* worker = m_workers.emplace_back(AllocateWorker(i));
			worker->thread = std::thread(std::bind(&JobSystem2::SpawnWorker, this, i));
			
			worker->workQueue.Allocate(NumMaxJobsPerQueue);

			PlatformThread::AssignThreadToCore(worker->thread.native_handle(), 1ull << i);
			PlatformThread::SetThreadPriority(worker->thread.native_handle(), ThreadPriority::High);

			std::string threadName = std::format("Volt::Worker {}", i);
			PlatformThread::SetThreadName(worker->thread.native_handle(), threadName);
		}

		// Notify all threads that they are allowed to run.
		m_wakeCondition.notify_all();
    }

	void JobSystem2::Shutdown()
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

	void JobSystem2::SpawnWorker(uint32_t workerId)
	{
		// Wait here for all threads to be created.
		{
			std::unique_lock<std::mutex> spawnLock(m_wakeMutex);
			m_wakeCondition.wait(spawnLock);
		}

		m_workerThreadIDToIndex[std::this_thread::get_id()] = workerId;

		while (m_isAlive.load(std::memory_order::relaxed))
		{
			Job2* jobPtr = TryGetJob(workerId);

			if (jobPtr)
			{
				{
					VT_PROFILE_SCOPE(jobPtr->GetName().data());
					jobPtr->Execute();
				}

				FinishJob(jobPtr);
			}
			else if (!FlushWaitingList())
			{
				std::unique_lock<std::mutex> lock(m_wakeMutex);
				m_wakeCondition.wait(lock);
			}
		}
	}

	Job2* JobSystem2::TryGetJob(uint32_t workerId)
	{
		auto& worker = m_workers.at(workerId);

		Job2* job = nullptr;
		if (!worker->workQueue.Pop(job))
		{
			uint32_t currentQueue = (workerId + 1) % m_numWorkers;

			while (currentQueue != workerId)
			{
				auto& stealingQueue = m_workers.at(currentQueue)->workQueue;
				if (stealingQueue.Pop(job))
				{
					return job;
				}

				currentQueue = (currentQueue + 1) % m_numWorkers;
			}
		}

		return job;
	}

	JobSystem2::JobWorker* JobSystem2::AllocateWorker(uint32_t workerId)
	{
		constexpr size_t WorkerSize = sizeof(JobWorker);
		void* allocatedPtr = m_workerAllocator.Allocate(WorkerSize);
		JobWorker* worker = new(allocatedPtr) JobWorker();

		return worker;
	}

	void JobSystem2::FinishJob(Job2* jobPtr)
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

	JobCounter* JobSystem2::AllocateCounter(bool initializeWithRef)
	{
		JobCounter* counter = m_counterAllocator.Allocate();

		if (initializeWithRef)
		{
			counter->IncRef();
		}

		return counter;
	}

	Job2* JobSystem2::AllocateJob()
	{
		Job2* job = m_jobAllocator.Allocate();
		job->IncRef();

		return job;
	}

	void JobSystem2::FreeCounter(JobCounter* counter)
	{
		VT_ENSURE_MSG(counter->IsCompleted(), "Counter must be completed!");
		counter->Reset();
		m_counterAllocator.Free(counter);
	}

	void JobSystem2::FreeJob(Job2* job)
	{
		job->Reset();
		m_jobAllocator.Free(job);
	}

	void JobSystem2::PushToWaitingList(Job2* job)
	{
		m_waitingList.Push(job);
	}

	bool JobSystem2::FlushWaitingList()
	{
		if (m_waitingList.Size() == 0)
		{
			return false;
		}

		// Use a lock here to make sure that only one thread flushes at a time.
		std::scoped_lock lock{ m_waitingListMutex };

		Vector<Job2*> nonReadyJobs;
		nonReadyJobs.reserve(128);

		bool anyJobRun = false;

		Job2* jobPtr;
		while (m_waitingList.Pop(jobPtr))
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
			m_waitingList.Push(job);
		}

		return anyJobRun;
	}
}
