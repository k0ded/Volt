#pragma once

#include "Volt-FileSystem/Config.h"
#include "Volt-FileSystem/IOThreads/IORequest.h"
#include "Volt-FileSystem/IOThreads/IORequestAllocator.h"

#include <JobSystem/JobSystem.h>

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/WorkQueue.h>

namespace Volt
{
	class JobCounter;

	class IOThreads : public SubSystem
	{
	public:
		IOThreads();
		~IOThreads();

		template<typename RequestType, typename... Args>
		static IORequestResult<RequestType> SubmitRequest(Args&&... args);

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{DA15C38A-04E8-490D-ADCE-C73D2F689E64}"_guid)

	private:
		friend class IORequest;

		inline static constexpr size_t NumMaxIORequest = 4096;

		struct IOThread
		{
			VT_PROFILE_DECLARE_MUTEX(std::mutex, wakeMutex);
			std::thread thread;
		};

		struct QueuedIORequest
		{
			IORequest* request = nullptr;
			JobCounter* referencedCounter = nullptr;
		};

		void Initialize() override;
		void Shutdown() override;

		void AllocateThreads();

		void SpawnIOThread(uint32_t workerId);

		void FreeIORequest(IORequest* request);

		VTFS_API inline static IOThreads* s_instance = nullptr;

		std::atomic_bool m_isRunning = true;
		std::condition_variable_any m_wakeCondition;

		WorkQueue<QueuedIORequest, QueueThreadingPolicy::MPMC> m_ioRequestQueue;
		PagedAtomicArenaAllocator<IOThread, 16> m_ioThreadAllocator;
		InlineVector<IOThread*, 16> m_ioThreads;

		IORequestAllocator m_requestAllocator;
	};

	template<typename RequestType, typename... Args>
	static IORequestResult<RequestType> IOThreads::SubmitRequest(Args&&... args)
	{
		VT_ENSURE(s_instance != nullptr);

		RequestType* request = s_instance->m_requestAllocator.Allocate<RequestType>(std::forward<Args>(args)...);
		request->IncRef();

		JobCounterRef counter = JobSystem::CreateCounter();

		QueuedIORequest queuedRequest;
		queuedRequest.request = request;
		queuedRequest.referencedCounter = counter;
		queuedRequest.referencedCounter->Increment();

		// Note: Ensure that the result is created before the request is queued.
		IORequestResult<RequestType> result(counter, request);

		s_instance->m_ioRequestQueue.Emplace(queuedRequest);
		s_instance->m_wakeCondition.notify_all();

		return result;
	}
}
