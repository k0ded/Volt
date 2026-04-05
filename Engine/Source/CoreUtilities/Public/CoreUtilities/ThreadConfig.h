#pragma once

#include "CoreUtilities/Config.h"

struct ThreadConfig
{
	bool isWorkerThread : 1 = false;
	bool isIOThread : 1 = false;
	bool isMainThread : 1 = false;
	bool isInitialized : 1 = false;

	int32_t activeFiberId = -1;
};

namespace Threads
{
	VTCOREUTIL_API extern void InitializeThreadConfig(bool isWorkerThread, bool isIOThread, bool isMainThread = false);
	VTCOREUTIL_API extern const ThreadConfig& GetThreadConfig();
	VTCOREUTIL_API extern void SetFiberExecutionID(int32_t fiberId);
}
