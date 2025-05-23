#pragma once

#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Config.h"
#include "Volt-Platforms/ProcessHandle.h"
#include "Volt-Platforms/CrashContext.h"

#include <thread>
#include <condition_variable>

struct _EXCEPTION_POINTERS;

namespace Volt
{
	class CommandLineBuilder;

	class VTPL_API WindowsCrashReportingThread
	{
	public:
		WindowsCrashReportingThread(bool isEnabled);
		~WindowsCrashReportingThread();

		void NotifyCrash(_EXCEPTION_POINTERS* exceptionInfo, const CommandLineBuilder& commandLineBuilder);

	private:
		void RunThread();
		void LaunchCrashReportClient();
		bool GenerateAndSerializeMiniDump();
		void HandleCrash();
		std::string CreateExceptionString();

		std::thread m_thread;
		std::atomic_bool m_isRunning = true;
		std::atomic_bool m_hasCrashed = false;
		std::condition_variable m_conditionVariable;
		std::mutex m_mutex;

		std::condition_variable m_crashingThreadConditionVariable;
		std::mutex m_crashingThreadMutex;

		bool m_isEnabled;

		// Crash reporter
		ProcessHandle m_crashReporterProcessHandle;
		uint32_t m_crashReporterProcessID;
		void* m_crashReporterWritePipe = nullptr;
		void* m_crashReporterReadPipe = nullptr;

		// Crash info
		_EXCEPTION_POINTERS* m_exceptionInfo = nullptr;
		unsigned long m_crashingThread = 0;
		void* m_crashingThreadHandle = nullptr;
		
		std::string m_crashingThreadStackTrace;
		std::string m_crashCommandLine;
		std::string m_crashTimestamp;
		CrashContext* m_crashContext = nullptr;
	};
}
#endif
