#include "Windows/WindowsCrashReportingThread.h"

#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Platform.h"

#include <LogModule/Log.h>

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/CommandLineBuilder.h>

#include <cpptrace/cpptrace.hpp>

#include <filesystem>
#include <sstream>

namespace Volt
{
	inline std::string GetTimestampString()
	{
		auto currentTime = std::time(nullptr);
		tm timeInfo;

		const auto localTimeError = localtime_s(&timeInfo, &currentTime);
		if (localTimeError != 0)
		{
			return "NULL";
		}

		std::ostringstream sstream;
		sstream << std::put_time(&timeInfo, "%Y-%m-%d_%H-%M");
		return sstream.str();
	}

	WindowsCrashReportingThread::WindowsCrashReportingThread(bool isEnabled)
		: m_isEnabled(isEnabled)
	{
		if (isEnabled)
		{
			m_thread = std::thread(&WindowsCrashReportingThread::RunThread, this);
			PlatformThread::SetThreadName(m_thread.native_handle(), "VoltCrashReportingThread");
			PlatformThread::SetThreadPriority(m_thread.native_handle(), ThreadPriority::Low);

			LaunchCrashReportClient();

			m_crashContext = new CrashContext();
		}
	}

	WindowsCrashReportingThread::~WindowsCrashReportingThread()
	{
		if (m_isEnabled)
		{
			m_isRunning = false;
			m_conditionVariable.notify_one();

			m_thread.join();

			delete m_crashContext;
		}
	}

	void WindowsCrashReportingThread::NotifyCrash(_EXCEPTION_POINTERS* exceptionInfo, const CommandLineBuilder& commandLineBuilder)
	{
		if (m_isEnabled)
		{
			m_exceptionInfo = exceptionInfo;
			m_crashingThread = GetCurrentThreadId();
			m_crashingThreadHandle = GetCurrentThread();

			// Get timestamp
			m_crashTimestamp = GetTimestampString();

			// Create stack trace
			std::ostringstream strStream;
			cpptrace::generate_trace().print(strStream);
			m_crashingThreadStackTrace = strStream.str();

			// Command line for restarting.
			m_crashCommandLine = commandLineBuilder.GetAsString();

			// Flush logs to disk
			Log::Get().Flush();

			m_hasCrashed = true;
			m_conditionVariable.notify_one();

			std::unique_lock lock{ m_mutex };
			m_crashingThreadConditionVariable.wait(lock);
		}
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
				//GenerateAndSerializeMiniDump();
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
		m_crashContext->platformCrashContext = m_exceptionInfo;
		m_crashContext->crashingThreadId = m_crashingThread;

		// Stack trace
		memcpy_s(m_crashContext->stackTrace, CrashContext::MAX_STACK_TRACE_SIZE, m_crashingThreadStackTrace.data(), m_crashingThreadStackTrace.size());

		// User name
		const std::string activeUser = PlatformMisc::GetCurrentUserName();
		memcpy_s(m_crashContext->userName, CrashContext::MAX_USER_NAME_SIZE, activeUser.data(), activeUser.size());

		// Timestamp
		memcpy_s(m_crashContext->timestamp, CrashContext::MAX_TIMESTAMP_SIZE, m_crashTimestamp.data(), m_crashTimestamp.size());

		// Command line
		memcpy_s(m_crashContext->commandLine, CrashContext::MAX_COMMAND_LINE_SIZE, m_crashCommandLine.data(), m_crashCommandLine.size());

		const uint8_t* dataPtr = reinterpret_cast<uint8_t*>(m_crashContext);
		const size_t dataSize = sizeof(CrashContext);

		PlatformProcess::WritePipe(m_crashReporterWritePipe, dataPtr, static_cast<uint32_t>(dataSize));
	}
}
#endif
