#include "FileSystemModule/IOThreads/IOThreads.h"

#include <PlatformsModule/Platform.h>

#include <JobSystem/JobSystem.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/ThreadConfig.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(IOThreads, Minimal, PreEngine);

	IOThreads::IOThreads()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;
	}

	IOThreads::~IOThreads()
	{
		s_instance = nullptr;
	}

	void IOThreads::Initialize()
	{
		m_ioRequestQueue.Allocate(NumMaxIORequest);
		AllocateThreads();
	}

	void IOThreads::Shutdown()
	{
		m_isRunning = false;
		m_wakeCondition.notify_all();

		for (IOThread* ioThread : m_ioThreads)
		{
			ioThread->thread.join();
			m_ioThreadAllocator.Free(ioThread);
		}
	}

	void IOThreads::AllocateThreads()
	{
		const uint32_t numIOThreads = PlatformMisc::GetNumberOfLogicalCores();

		for (uint32_t i = 0; i < numIOThreads; ++i)
		{
			IOThread* ioThread = m_ioThreads.emplace_back(m_ioThreadAllocator.Allocate());
			ioThread->thread = std::thread(std::bind(&IOThreads::SpawnIOThread, this, i));

			String threadName = FormatString("Volt::IOThread {}", i);
			PlatformThread::SetThreadName(ioThread->thread.native_handle(), threadName);
		}
	}

	void IOThreads::SpawnIOThread(uint32_t workerId)
	{
		Threads::InitializeThreadConfig(false, true);

		IOThread& workerData = *m_ioThreads[workerId];

		while (m_isRunning.load(std::memory_order::relaxed))
		{
			QueuedIORequest request;
			while (m_ioRequestQueue.Pop(request))
			{
				VT_PROFILE_SCOPE(request.request->GetName().data());

				request.request->Execute();
				request.referencedCounter->Decrement();
				request.referencedCounter->DecRef();
				request.request->DecRef();
			}

			std::unique_lock lock(workerData.wakeMutex);
			VT_PROFILE_LOCK_MARK(workerData.wakeMutex);
			m_wakeCondition.wait(lock, [this]()
			{
				return !m_isRunning.load(std::memory_order::relaxed) ||
					m_ioRequestQueue.Size() > 0;
			});
		}
	}

	void IOThreads::FreeIORequest(IORequest* request)
	{
		m_requestAllocator.Free(request);
	}

	void IOThreads::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<JobSystem>();
	}
}
