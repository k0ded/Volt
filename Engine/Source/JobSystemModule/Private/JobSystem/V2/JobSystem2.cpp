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

	void JobSystem2::RunJob(Job2* job)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE(s_instance);

		const uint32_t nextQueueToPush = s_instance->m_nextQueueToPush.fetch_add(1, std::memory_order::relaxed) % s_instance->m_numWorkers;
		s_instance->m_workers.at(nextQueueToPush)->workQueue.Emplace(job);
		s_instance->m_wakeCondition.notify_all();
	}

	void JobSystem2::Initialize()
    {
		m_isAlive = true;
		const uint32_t hardwareConcurrency = PlatformMisc::GetNumberOfPhysicalCores();
		m_numWorkers = hardwareConcurrency;

		m_workers.reserve(m_numWorkers);

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

		while (m_isAlive.load(std::memory_order::relaxed))
		{
			Job2* jobPtr = TryGetJob(workerId);

			if (jobPtr)
			{
				{
					VT_PROFILE_SCOPE(jobPtr->GetName().data());
					jobPtr->Execute();
				}

				m_jobAllocator.Free(jobPtr);
			}
			else
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
}
