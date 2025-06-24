#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Windows/WindowsPlatformMisc.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/StringUtility.h>
#include <CoreUtilities/Malloc.h>
#include <CoreUtilities/Math/Math.h>

#include <combaseapi.h>
#include <lmcons.h>
#include <winnt.h>

namespace Volt
{
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

	std::string WindowsPlatformMisc::GetSystemErrorMessage(int32_t error)
	{
		if (error == 0)
		{
			error = GetLastError();
		}

		LPWSTR strBuffer = nullptr;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, error, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPWSTR)&strBuffer, 0, NULL);

		std::string result = Utility::ToString(std::wstring(strBuffer));
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

	std::string WindowsPlatformMisc::GetCurrentUserName()
	{
		char name[UNLEN + 1];
		DWORD size = UNLEN + 1;

		if (GetUserNameA(name, &size))
		{
			return std::string(name);
		}

		return std::string("Unnamned");
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
}
#endif
