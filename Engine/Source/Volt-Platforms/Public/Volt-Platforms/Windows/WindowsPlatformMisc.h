#pragma once

#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Config.h"

#include <CoreUtilities/VoltGUID.h>

#include <cstdint>
#include <string>

namespace Volt
{
	class VTPL_API WindowsPlatformMisc
	{
	public:
		static void RequestApplicationExit(bool forceExit, uint32_t exitCode);
		static std::string GetSystemErrorMessage(int32_t error);
		static bool IsDebuggerPresent();
		static VoltGUID GenerateGUID();
	};
}

#endif
