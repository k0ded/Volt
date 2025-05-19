#pragma once

#include <cstdint>

namespace Volt
{
	struct CrashContext
	{
		static constexpr uint32_t MAX_STACK_TRACE_SIZE = 8192;

		void* platformCrashContext;
		uint32_t crashingThreadId;

		uint32_t stackTraceSize;
		char stackTrace[MAX_STACK_TRACE_SIZE];
	};
}
