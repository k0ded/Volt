#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Windows/WindowsPlatformMisc.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/StringUtility.h>

#include <combaseapi.h>

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
}
#endif
