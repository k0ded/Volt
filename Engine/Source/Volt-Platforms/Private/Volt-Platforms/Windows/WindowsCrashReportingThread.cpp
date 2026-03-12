#include "Windows/WindowsCrashReportingThread.h"

#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Platform.h"

#include <LogModule/Log.h>

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/CommandLineBuilder.h>
#include <CoreUtilities/Archive/MemoryArchive.h>

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

	void WindowsCrashReportingThread::NotifyCrash(_EXCEPTION_POINTERS* exceptionInfo, const CommandLineBuilder& commandLineBuilder, const CrashReporterConnectionInfo& connectionInfo)
	{
		if (m_isEnabled)
		{
			m_exceptionInfo = exceptionInfo;
			m_crashingThread = GetCurrentThreadId();
			m_crashingThreadHandle = GetCurrentThread();
			m_crashReporterConnectionInfo = connectionInfo;

			// Get timestamp
			m_crashTimestamp = GetTimestampString();

			// Create stack trace
			std::ostringstream strStream;
			cpptrace::generate_trace().print(strStream);
			m_crashingThreadStackTrace = strStream.str();

			// Command line for restarting.
			m_crashCommandLine = commandLineBuilder.GetAsString();

			// Flush logs to disk
			if (Log::IsInitialized())
			{
				Log::Get().Flush();
			}

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
		// As we at this point might be inside the binaries directory, we must also check if the crash reporter lies in the current directory.
		auto crashReportClientFilepath = std::filesystem::current_path() / "Binaries\\CrashReportClient.exe";
		if (!FileSystem::Exists(crashReportClientFilepath))
		{
			crashReportClientFilepath = std::filesystem::current_path() / "CrashReportClient.exe";
		}

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
		CrashContext crashContext;

		crashContext.platformCrashContext = m_exceptionInfo;
		crashContext.crashingThreadId = m_crashingThread;
		crashContext.stackTrace = m_crashingThreadStackTrace;
		crashContext.username = PlatformMisc::GetCurrentUserName();
		crashContext.timestamp = m_crashTimestamp;
		crashContext.commandLine = m_crashCommandLine;
		crashContext.errorString = CreateExceptionString();
		crashContext.serverURL = m_crashReporterConnectionInfo.serverURL;
		crashContext.serverUser = m_crashReporterConnectionInfo.serverUser;
		crashContext.serverPassword = m_crashReporterConnectionInfo.serverPassword;

		MemoryWriter archive{};
		archive << crashContext;
		archive.Close();

		PlatformProcess::WritePipe(m_crashReporterWritePipe, reinterpret_cast<const uint8_t*>(archive.GetData()), static_cast<uint32_t>(archive.GetSize()));
	}

	std::string WindowsCrashReportingThread::CreateExceptionString()
	{
		std::string errorString = "Unhandled Exception: ";

#define HANDLE_CASE(value) case value: errorString += #value; break

		switch (m_exceptionInfo->ExceptionRecord->ExceptionCode)
		{
			case EXCEPTION_ACCESS_VIOLATION:
			{
				errorString += "EXCEPTION_ACCESS_VIOLATION ";
				if (m_exceptionInfo->ExceptionRecord->ExceptionInformation[0] == 0)
				{
					errorString += "reading address ";
				}
				else if (m_exceptionInfo->ExceptionRecord->ExceptionInformation[0] == 1)
				{
					errorString += "writing address ";
				}
				errorString += std::format("{}", m_exceptionInfo->ExceptionRecord->ExceptionInformation[1]);
				break;
			}

			HANDLE_CASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
			HANDLE_CASE(EXCEPTION_DATATYPE_MISALIGNMENT);
			HANDLE_CASE(EXCEPTION_FLT_DENORMAL_OPERAND);
			HANDLE_CASE(EXCEPTION_FLT_DIVIDE_BY_ZERO);
			HANDLE_CASE(EXCEPTION_FLT_INVALID_OPERATION);
			HANDLE_CASE(EXCEPTION_ILLEGAL_INSTRUCTION);
			HANDLE_CASE(EXCEPTION_INT_DIVIDE_BY_ZERO);
			HANDLE_CASE(EXCEPTION_PRIV_INSTRUCTION);
			HANDLE_CASE(EXCEPTION_STACK_OVERFLOW);

			default:
				errorString += std::format("{}", m_exceptionInfo->ExceptionRecord->ExceptionCode);
		}

		return errorString;
	}

}
#endif
