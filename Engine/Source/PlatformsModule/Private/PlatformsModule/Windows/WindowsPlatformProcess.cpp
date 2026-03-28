#include "PlatformsModule/Windows/WindowsPlatformProcess.h"

#ifdef VT_PLATFORM_WINDOWS

#include "PlatformsModule/Platform.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>

namespace Volt
{
	ProcessHandle WindowsPlatformProcess::CreateProc(const Filesystem::Path& processFilepath, const String& parameters, bool launchAsDetached, bool launchAsHidden, uint32_t* outProcessId, const Filesystem::Path& workingDirectory /*= ""*/, void* pipeWriteChild /*= nullptr*/, void* pipeReadChild /*= nullptr*/, void* stdErrChild /*= nullptr*/)
	{
		uint32_t processCreateFlags = NORMAL_PRIORITY_CLASS;

		if (launchAsDetached)
		{
			processCreateFlags |= DETACHED_PROCESS;
		}

		uint32_t windowFlags = 0;
		uint16_t windowShowFlags = SW_HIDE;

		if (launchAsHidden)
		{
			windowFlags = STARTF_USESHOWWINDOW;
			windowShowFlags = SW_SHOWMINNOACTIVE;
		}

		if (pipeWriteChild != nullptr || pipeReadChild != nullptr || stdErrChild != nullptr)
		{
			windowFlags |= STARTF_USESTDHANDLES;
		}

		STARTUPINFO windowStartupInfo =
		{
			sizeof(STARTUPINFO), // cb
			NULL, NULL, NULL, // lpReserved, lpDesktop, lpTitle
			(DWORD)CW_USEDEFAULT, // dwX
			(DWORD)CW_USEDEFAULT, // dwY
			(DWORD)CW_USEDEFAULT, // dwXSize
			(DWORD)CW_USEDEFAULT, // dwYSize
			0, 0, 0, // dwXCountChars, dwYCountChars, dwFillAttribute
			windowFlags, // dwFlags
			windowShowFlags, // wShowWindow
			0, NULL, // cbReserved2, lpReserved2
			HANDLE(pipeReadChild), // hStdInput
			HANDLE(pipeWriteChild), // hStdOutput
			HANDLE(stdErrChild) // hStdError
		};

		bool shouldInheritHandles = (windowFlags & STARTF_USESTDHANDLES) != 0;

		WString processCommandLine = FormatString(L"\"{}\" {}", processFilepath.ToWString(), parameters);

		PROCESS_INFORMATION processInfo;
		if (!CreateProcess(NULL, processCommandLine.data(), nullptr, nullptr, shouldInheritHandles, processCreateFlags, NULL, workingDirectory.IsEmpty() ? NULL : workingDirectory.CStr(), &windowStartupInfo, &processInfo))
		{
			DWORD lastError = GetLastError();
			const String errorString = PlatformMisc::GetSystemErrorMessage(lastError);
			
			if (::IsDebuggerPresent())
			{
				OutputDebugStringA(errorString.c_str());
			}

			if (outProcessId)
			{
				*outProcessId = 0;
			}

			return ProcessHandle();
		}

		if (outProcessId)
		{
			*outProcessId = processInfo.dwProcessId;
		}

		::CloseHandle(processInfo.hThread);
		return ProcessHandle(processInfo.hProcess);
	}

	ProcessHandle WindowsPlatformProcess::OpenProc(uint32_t processId)
	{
		return ProcessHandle(::OpenProcess(PROCESS_ALL_ACCESS, 0, processId));
	}

	ProcessHandle WindowsPlatformProcess::OpenProcRestricted(uint32_t processId)
	{
		return ProcessHandle(::OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, 0, processId));
	}

	uint32_t WindowsPlatformProcess::GetCurrentProcessId()
	{
		return ::GetCurrentProcessId();
	}

	bool WindowsPlatformProcess::IsProcRunning(ProcessHandle& processHandle)
	{
		uint32_t waitResult = ::WaitForSingleObject(processHandle.Get(), 0);
		return waitResult == WAIT_TIMEOUT;
	}

	void WindowsPlatformProcess::CloseProc(ProcessHandle& processHandle)
	{
		if (processHandle.IsValid())
		{
			::CloseHandle(processHandle.Get());
			processHandle.Reset();
		}
	}

	bool WindowsPlatformProcess::CreatePipe(void*& outReadPipe, void*& outWritePipe, bool writePipeLocal)
	{
		SECURITY_ATTRIBUTES secAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, true };

		if (!::CreatePipe(&outReadPipe, &outWritePipe, &secAttr, 0))
		{
			return false;
		}

		if (!::SetHandleInformation(writePipeLocal ? outWritePipe : outReadPipe, HANDLE_FLAG_INHERIT, 0))
		{
			return false;
		}

		return true;
	}

	void WindowsPlatformProcess::ClosePipe(void* readPipe, void* writePipe)
	{
		if (readPipe != nullptr && readPipe != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(readPipe);
		}

		if (writePipe != nullptr && writePipe != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(writePipe);
		}
	}

	bool WindowsPlatformProcess::WritePipe(void* writePipe, const uint8_t* data, const uint32_t dataSize)
	{
		if (dataSize == 0 || writePipe == nullptr)
		{
			return false;
		}

		uint32_t dataWritten = 0;
		bool didWrite = !!WriteFile(writePipe, data, dataSize, (::DWORD*)&dataWritten, nullptr);

		return didWrite;
	}

	bool WindowsPlatformProcess::ReadPipe(void* readPipe, Vector<uint8_t>& outData)
	{
		uint32_t numBytesAvailable = 0;
		if (::PeekNamedPipe(readPipe, NULL, 0, NULL, (::DWORD*)&numBytesAvailable, NULL))
		{
			if (numBytesAvailable > 0)
			{
				outData.resize_uninitialized(numBytesAvailable);
				uint32_t numBytesRead = 0;
				if (::ReadFile(readPipe, outData.data(), numBytesAvailable, (::DWORD*)&numBytesRead, NULL))
				{
					outData.resize_uninitialized(numBytesRead);
				}

				return true;
			}

			outData.clear();
		}
		return false;
	}

}
#endif
