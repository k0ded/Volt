#pragma once

#include <cstdint>

namespace Volt
{
	struct CrashContext
	{
		static constexpr uint32_t MAX_STACK_TRACE_SIZE = 8192;
		static constexpr uint32_t MAX_USER_NAME_SIZE = 1024;
		static constexpr uint32_t MAX_TIMESTAMP_SIZE = 256;
		static constexpr uint32_t MAX_COMMAND_LINE_SIZE = 1024;

		void* platformCrashContext;
		uint32_t crashingThreadId;
		char stackTrace[MAX_STACK_TRACE_SIZE];
		char userName[MAX_USER_NAME_SIZE];
		char timestamp[MAX_TIMESTAMP_SIZE];
		char commandLine[MAX_COMMAND_LINE_SIZE];
	};
}
