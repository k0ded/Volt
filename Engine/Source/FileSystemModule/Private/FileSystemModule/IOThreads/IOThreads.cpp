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

		for (IOThread* ioThread : m_ioThreads)
		{
			m_workAvailableSemaphore.release();

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

		while (m_isRunning.load(std::memory_order::relaxed))
		{
			QueuedIORequest request;
			while (m_ioRequestQueue.Pop(request))
			{
				VT_PROFILE_SCOPE(request.request->GetName().data());
				ExecuteIORequest(std::move(request));
			}

			m_workAvailableSemaphore.acquire();
		}
	}

	void IOThreads::FreeIORequest(IORequest* request)
	{
		m_requestAllocator.Free(request);
	}

	void IOThreads::ExecuteIORequest(QueuedIORequest&& request)
	{
		request.request->Execute();
		request.referencedCounter->Decrement();
		request.referencedCounter->DecRef();
		request.request->DecRef();
	}

	void IOThreads::QueueOrExecuteIORequest(QueuedIORequest&& request)
	{
		const ThreadConfig& threadConfig = Threads::GetThreadConfig();

		// If we are coming from an IO thread, we'll execute the request directly to not get deadlocked.
		if (threadConfig.isIOThread)
		{
			ExecuteIORequest(std::move(request));
		}
		else
		{
			m_ioRequestQueue.Emplace(request);
			m_workAvailableSemaphore.release();
		}
	}

	void IOThreads::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<JobSystem>();
	}
}
