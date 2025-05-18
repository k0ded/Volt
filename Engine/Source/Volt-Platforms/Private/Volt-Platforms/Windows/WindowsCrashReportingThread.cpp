#include "Windows/WindowsCrashReportingThread.h"

#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Platform.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/CommandLineBuilder.h>

#include <filesystem>

namespace Volt
{
	WindowsCrashReportingThread::WindowsCrashReportingThread(bool isEnabled)
		: m_isEnabled(isEnabled)
	{
		if (isEnabled)
		{
			m_thread = std::thread(&WindowsCrashReportingThread::RunThread, this);
			PlatformThread::SetThreadName(m_thread.native_handle(), "VoltCrashReportingThread");
			PlatformThread::SetThreadPriority(m_thread.native_handle(), ThreadPriority::Low);

			LaunchCrashReportClient();
		}
	}

	WindowsCrashReportingThread::~WindowsCrashReportingThread()
	{
		if (m_isEnabled)
		{
			m_isRunning = false;
			m_conditionVariable.notify_one();

			m_thread.join();
		}
	}

	void WindowsCrashReportingThread::NotifyCrash(_EXCEPTION_POINTERS* exceptionInfo)
	{
		m_exceptionInfo = exceptionInfo;
		m_crashingThread = GetCurrentThreadId();
		m_crashingThreadHandle = GetCurrentThread();

		m_hasCrashed = true;
		m_conditionVariable.notify_one();

		std::unique_lock lock{ m_mutex };
		m_crashingThreadConditionVariable.wait(lock);
	}

	void WindowsCrashReportingThread::RunThread()
	{
		while (m_isRunning)
		{
			std::unique_lock lock{ m_mutex };
			m_conditionVariable.wait(lock, [&]() { return m_hasCrashed || !m_isRunning; });

			if (m_hasCrashed)
			{
				HandleCrash();
				GenerateAndSerializeMiniDump();
				m_crashingThreadConditionVariable.notify_one();
				break;
			}
		}
	}

	void WindowsCrashReportingThread::LaunchCrashReportClient()
	{
		const auto crashReportClientFilepath = std::filesystem::current_path() / "Binaries\\CrashReportClient.exe";
		if (FileSystem::Exists(crashReportClientFilepath))
		{
			void *pipeChildInRead, *pipeChildInWrite, *pipeChildOutRead, *pipeChildOutWrite;

			if (!PlatformProcess::CreatePipe(pipeChildInRead, pipeChildInWrite, true) || !PlatformProcess::CreatePipe(pipeChildOutRead, pipeChildOutWrite, false))
			{
				return;
			}

			m_crashReporterWritePipe = pipeChildInWrite;
			m_crashReporterReadPipe = pipeChildOutRead;

			CommandLineBuilder commandLineBuilder;
			commandLineBuilder.AddArgument("monitorprocess", std::to_string(PlatformProcess::GetCurrentProcessId()));
			commandLineBuilder.AddArgument("readpipe", std::format("{}", reinterpret_cast<uintptr_t>(pipeChildInRead)));
			commandLineBuilder.AddArgument("writepipe", std::format("{}", reinterpret_cast<uintptr_t>(pipeChildOutWrite)));
			//commandLineBuilder.AddArgument("waitfordebugger");

			m_crashReporterProcessHandle = PlatformProcess::CreateProc(
				crashReportClientFilepath,
				commandLineBuilder.GetAsString(),
				true, false, &m_crashReporterProcessID,
				"",
				pipeChildInRead);
		}
	}

	bool WindowsCrashReportingThread::GenerateAndSerializeMiniDump()
	{
		const std::filesystem::path dumpFilepath = "Volt.dmp";

		HANDLE dumpFileHandle = CreateFileW(dumpFilepath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (dumpFileHandle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		MINIDUMP_EXCEPTION_INFORMATION dumpExceptionInfo{};
		dumpExceptionInfo.ThreadId = m_crashingThread;
		dumpExceptionInfo.ExceptionPointers = m_exceptionInfo;
		dumpExceptionInfo.ClientPointers = true;

		MINIDUMP_TYPE dumpType = MiniDumpNormal;

		// Note: Full dumps
		//dumpType = MINIDUMP_TYPE(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData | MiniDumpWithThreadInfo);
	
		const bool writeResult = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFileHandle, dumpType, m_exceptionInfo ? &dumpExceptionInfo : NULL, NULL, NULL);
		CloseHandle(dumpFileHandle);

		return writeResult;
	}

	void WindowsCrashReportingThread::HandleCrash()
	{
		const std::string str = "Crashed!";
		PlatformProcess::WritePipe(m_crashReporterWritePipe, reinterpret_cast<const uint8_t*>(str.c_str()), static_cast<uint32_t>(str.size()));
	}
}
#endif
