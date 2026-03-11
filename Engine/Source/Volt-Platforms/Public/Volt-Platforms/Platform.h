#pragma once
// Contains type redefinitions for all platform specific stuff

#ifdef VT_PLATFORM_WINDOWS
#include "Volt-Platforms/Windows/WindowsCrashReportingThread.h"
#include "Volt-Platforms/Windows/WindowsPlatformMisc.h"
#include "Volt-Platforms/Windows/WindowsPlatformProcess.h"
#include "Volt-Platforms/Windows/WindowsPlatformThread.h"
#include "Volt-Platforms/Windows/WindowsPlatformAtomics.h"
#include "Volt-Platforms/Windows/WindowsPlatformTime.h"
#include "Volt-Platforms/Windows/WindowsPlatformMutex.h"
#include "Volt-Platforms/Windows/WindowsPlatformMemory.h"
#include "Volt-Platforms/Common/CommonPlatformFTPClient.h"

namespace Volt
{
	using CrashReportingThread = WindowsCrashReportingThread;
	using PlatformMisc = WindowsPlatformMisc;
	using PlatformProcess = WindowsPlatformProcess;
	using PlatformThread = WindowsPlatformThread;
	using PlatformAtomics = WindowsPlatformAtomics;
	using PlatformTime = WindowsPlatformTime;
	using PlatformMutex = WindowsPlatformMutex;
	using PlatformMemory = WindowsPlatformMemory;
	using PlatformFTPClient = CommonPlatformFTPClient;
}
#endif
