#pragma once

#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/ProcessHandle.h"
#include "Volt-Platforms/Config.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Filesystem/Path.h>

namespace Volt
{
	class VTPL_API WindowsPlatformProcess
	{
	public:
		static ProcessHandle CreateProc(const Filesystem::Path& processFilepath, const String& parameters, bool launchAsDetached, bool launchAsHidden, uint32_t* outProcessId, const Filesystem::Path& workingDirectory = "", void* pipeWriteChild = nullptr, void* pipeReadChild = nullptr, void* stdErrChild = nullptr);
		static ProcessHandle OpenProc(uint32_t processId);
		static ProcessHandle OpenProcRestricted(uint32_t processId);
		static uint32_t GetCurrentProcessId();
		static bool IsProcRunning(ProcessHandle& processHandle);
		static void CloseProc(ProcessHandle& processHandle);

		static bool CreatePipe(void*& outReadPipe, void*& outWritePipe, bool writePipeLocal);
		static void ClosePipe(void* readPipe, void* writePipe);
		static bool WritePipe(void* writePipe, const uint8_t* data, const uint32_t dataSize);
		static bool ReadPipe(void* readPipe, Vector<uint8_t>& outData);
	};
}
#endif
