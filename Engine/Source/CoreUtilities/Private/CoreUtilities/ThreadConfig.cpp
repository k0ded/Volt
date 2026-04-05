#include "cupch.h"

#include "CoreUtilities/ThreadConfig.h"
#include "CoreUtilities/VoltAssert.h"

namespace Threads
{
	thread_local ThreadConfig g_threadConfig;

	void InitializeThreadConfig(bool isWorkerThread, bool isIOThread, bool isMainThread /*= false*/)
	{
		g_threadConfig.isWorkerThread = isWorkerThread;
		g_threadConfig.isIOThread = isIOThread;
		g_threadConfig.isMainThread = isMainThread;
		g_threadConfig.isInitialized = true;
	}

	const ThreadConfig& GetThreadConfig()
	{
		VT_FATAL(g_threadConfig.isInitialized);
		return g_threadConfig;
	}

	void SetFiberExecutionID(int32_t fiberId)
	{
		VT_FATAL(g_threadConfig.isInitialized);
		g_threadConfig.activeFiberId = fiberId;
	}
}
