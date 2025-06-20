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

		template<typename Func>
		static Job2* CreateJob(std::string_view jobName, Func&& func);

		static void RunJob(Job2* job);

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

		inline static constexpr size_t NumMaxWorkers = 64;
		inline static constexpr size_t NumMaxJobsPerQueue = 4096;
		inline static JobSystem2* s_instance = nullptr;

		std::atomic<bool> m_isAlive;
		std::atomic<uint32_t> m_nextQueueToPush = 0;
		std::condition_variable m_wakeCondition;
		std::mutex m_wakeMutex;
		uint32_t m_numWorkers = 0;

		Vector<JobWorker*> m_workers;
		LinearAllocator<sizeof(JobWorker) * NumMaxWorkers> m_workerAllocator;
		JobAllocator2<Job2, NumMaxJobsPerQueue> m_jobAllocator;
	};

	template<typename Func>
	Job2* JobSystem2::CreateJob(std::string_view jobName, Func&& func)
	{
		VT_PROFILE_FUNCTION();

		Job2* newJob = s_instance->m_jobAllocator.Allocate();
		newJob->Create(jobName, func);

		return newJob;
	}
}
