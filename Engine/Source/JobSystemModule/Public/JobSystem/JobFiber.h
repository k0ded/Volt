#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/FiberCommon.h"
#include "JobSystem/FiberContext.h"

#include <CoreUtilities/String/VoltString.h>

namespace Volt
{
	class Job;

	class JobFiber
	{
	public:
		VTJS_API JobFiber(const String& fiberName);

		VTJS_API bool ExecuteJob(Job* job);
		VTJS_API void ContinueExecution();
		VTJS_API void Free();

	private:
		friend void ExecuteFiber(void* userdata);
		friend class JobSystem;

		FiberContext m_executionContext;

		FiberStack m_stack;
		Job* m_currentJob = nullptr;
		String m_name;
	};
}
