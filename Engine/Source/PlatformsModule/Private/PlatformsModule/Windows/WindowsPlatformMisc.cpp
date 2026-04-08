#ifdef VT_PLATFORM_WINDOWS

#include "PlatformsModule/Windows/WindowsPlatformMisc.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/Malloc.h>
#include <CoreUtilities/Math/Math.h>
#include <CoreUtilities/VoltAssert.h>

#include <combaseapi.h>
#include <lmcons.h>
#include <winnt.h>

namespace Volt
{
	LONG CALLBACK StackOverflowHandler(EXCEPTION_POINTERS* info)
	{
		if (info->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
		{
			VT_FATAL_MSG(false, "StackOverflow: If inside a Job callstack, try increasing the FiberStackSize.\n");
			return EXCEPTION_EXECUTE_FAULT;
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}

	void WindowsPlatformMisc::RequestApplicationExit(bool forceExit, uint32_t exitCode)
	{
		if (forceExit)
		{
			// Force exit of the application, will not call any global destructors.
			TerminateProcess(GetCurrentProcess(), exitCode);
		}
		else
		{
			PostQuitMessage(exitCode);
		}
	}

	void WindowsPlatformMisc::CreateExternalConsole()
	{
		AllocConsole();
		FILE* newstdin = nullptr;
		FILE* newstdout = nullptr;
		FILE* newstderr = nullptr;

		freopen_s(&newstdin, "conin$", "r", stdin);
		freopen_s(&newstdout, "conout$", "w", stdout);
		freopen_s(&newstderr, "conout$", "w", stderr);
	}

	String WindowsPlatformMisc::GetSystemErrorMessage(int32_t error)
	{
		if (error == 0)
		{
			error = GetLastError();
		}

		LPSTR strBuffer = nullptr;
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, error, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPSTR)&strBuffer, 0, NULL);

		String result(strBuffer);
		LocalFree(strBuffer);

		return result;
	}

	bool WindowsPlatformMisc::IsDebuggerPresent()
	{
		return ::IsDebuggerPresent();
	}

	VoltGUID WindowsPlatformMisc::GenerateGUID()
	{
		GUID newGuid;
		HRESULT result = CoCreateGuid(&newGuid);
		VT_UNUSED(result);
		assert(SUCCEEDED(result));

		return VoltGUID::Construct(newGuid.Data1, newGuid.Data2, newGuid.Data3,
			newGuid.Data4[0], newGuid.Data4[1], newGuid.Data4[2], newGuid.Data4[3], newGuid.Data4[4],
			newGuid.Data4[5], newGuid.Data4[6], newGuid.Data4[7]);
	}

	String WindowsPlatformMisc::GetCurrentUserName()
	{
		char name[UNLEN + 1];
		DWORD size = UNLEN + 1;

		if (GetUserNameA(name, &size))
		{
			return String(name);
		}

		return String("Unnamned");
	}

	static void QueryCPUInfo(uint32_t& outNumCores, uint32_t& outNumLogicalCores)
	{
		outNumCores = 0;
		outNumLogicalCores = 0;

		uint8_t* bufferPtr = nullptr;
		DWORD bufferSize = 0;

		if (GetLogicalProcessorInformationEx(RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)bufferPtr, &bufferSize) == false)
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				bufferPtr = reinterpret_cast<uint8_t*>(Memory::Malloc(bufferSize));

				if (GetLogicalProcessorInformationEx(RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)bufferPtr, &bufferSize))
				{
					uint8_t* infoPtr = bufferPtr;
					while (infoPtr < bufferPtr + bufferSize)
					{
						PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)infoPtr;

						if (!processorInfo)
						{
							break;
						}

						if (processorInfo->Relationship == RelationProcessorCore)
						{
							outNumCores++;

							for (int32_t groupIndex = 0; groupIndex < processorInfo->Processor.GroupCount; ++groupIndex)
							{
								outNumLogicalCores += Math::CountBits(processorInfo->Processor.GroupMask[groupIndex].Mask);
							}
						}

						infoPtr += processorInfo->Size;
					}
				}

				Memory::Free(bufferPtr);
			}
		}
	}

	uint32_t WindowsPlatformMisc::GetNumberOfPhysicalCores()
	{
		uint32_t numCores, numLogicalCores;
		QueryCPUInfo(numCores, numLogicalCores);
		
		return numCores;
	}
	
	uint32_t WindowsPlatformMisc::GetNumberOfLogicalCores()
	{
		uint32_t numCores, numLogicalCores;
		QueryCPUInfo(numCores, numLogicalCores);

		return numLogicalCores;
	}

	void WindowsPlatformMisc::SetupExceptionHandlers()
	{
		AddVectoredExceptionHandler(1, StackOverflowHandler);
	}
}
#endif
