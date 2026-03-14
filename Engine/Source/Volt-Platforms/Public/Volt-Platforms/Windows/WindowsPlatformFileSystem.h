#pragma once

#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Config.h"
#include "Volt-Platforms/FileHandle.h"

#include <CoreUtilities/Containers/ArrayView.h>

#include <filesystem>

namespace Volt
{
	class VTPL_API WindowsPlatformFileSystem
	{
	public:
		static FileHandle OpenFile(const std::filesystem::path& filepath, bool writeable, bool randomAccess);
		static FileHandle CreateFile(const std::filesystem::path& destinationFilepath);
		static void CloseFile(FileHandle fileHandle);

		static void ReadFile(FileHandle fileHandle, uint64_t numBytesToRead, void* outData, uint64_t outDataSize);
		static void WriteFile(FileHandle fileHandle, const void* data, uint64_t dataSize);
		static uint64_t GetFileSize(FileHandle fileHandle);
	};
}

#endif
