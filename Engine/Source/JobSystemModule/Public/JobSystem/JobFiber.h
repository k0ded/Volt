#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/FiberCommon.h"
#include "JobSystem/FiberContext.h"

namespace Volt
{
	class Job;

	class JobFiber
	{
	public:
		VTJS_API JobFiber(const std::string& fiberName);

		VTJS_API bool ExecuteJob(Job* job);
		VTJS_API void ContinueExecution();
		VTJS_API void Free();

	private:
		friend void ExecuteFiber(void* userdata);
		friend class JobSystem;

		FiberContext m_executionContext;

		FiberStack m_stack;
		Job* m_currentJob = nullptr;
		std::string m_name;
	};
}
