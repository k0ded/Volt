#pragma once
// Contains type redefinitions for all platform specific stuff

#ifdef VT_PLATFORM_WINDOWS
#include "PlatformsModule/Windows/WindowsCrashReportingThread.h"
#include "PlatformsModule/Windows/WindowsPlatformMisc.h"
#include "PlatformsModule/Windows/WindowsPlatformProcess.h"
#include "PlatformsModule/Windows/WindowsPlatformThread.h"
#include "PlatformsModule/Windows/WindowsPlatformAtomics.h"
#include "PlatformsModule/Windows/WindowsPlatformTime.h"
#include "PlatformsModule/Windows/WindowsPlatformMutex.h"
#include "PlatformsModule/Windows/WindowsPlatformMemory.h"
#include "PlatformsModule/Windows/WindowsPlatformFileSystem.h"
#include "PlatformsModule/Common/CommonPlatformFTPClient.h"

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
	using PlatformFileSystem = WindowsPlatformFileSystem;
	using PlatformFTPClient = CommonPlatformFTPClient;

	using PlatformRecursiveDirectoryIterator = WindowsPlatformRecursiveDirectoryIterator;
	using PlatformDirectoryIterator = WindowsPlatformDirectoryIterator;
}
#endif
