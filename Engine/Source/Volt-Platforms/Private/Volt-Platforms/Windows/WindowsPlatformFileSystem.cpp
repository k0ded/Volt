#include "Volt-Platforms/Windows/WindowsPlatformFileSystem.h"

#ifdef VT_PLATFORM_WINDOWS

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/VoltAssert.h>

#undef CreateFile

namespace Volt
{
	FileHandle WindowsPlatformFileSystem::OpenFile(const std::filesystem::path& filepath, bool writeable, bool randomAccess)
	{
		HANDLE fileHandle = ::CreateFileW(
			filepath.c_str(),
			writeable ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			randomAccess ? FILE_FLAG_RANDOM_ACCESS : FILE_FLAG_SEQUENTIAL_SCAN,
			nullptr
		);

		return FileHandle(fileHandle);
	}

	FileHandle WindowsPlatformFileSystem::CreateFile(const std::filesystem::path& destinationFilepath)
	{
		HANDLE fileHandle = ::CreateFileW(
			destinationFilepath.c_str(),
			GENERIC_WRITE,
			FILE_SHARE_READ,
			nullptr,
			CREATE_ALWAYS,
			0,
			nullptr
		);

		return FileHandle(fileHandle);
	}

	void WindowsPlatformFileSystem::CloseFile(FileHandle fileHandle)
	{
		VT_ASSERT(fileHandle.IsValid());
		CloseHandle(fileHandle.Get());
	}

	void WindowsPlatformFileSystem::ReadFile(FileHandle fileHandle, uint64_t numBytesToRead, void* outData, uint64_t outDataSize)
	{
		VT_ASSERT(fileHandle.IsValid());
		VT_ENSURE_MSG(outDataSize >= numBytesToRead, "The output data must be at least 'numBytesToRead' large!");


		uint64_t totalBytesRead = 0;
		while (totalBytesRead < numBytesToRead)
		{
			DWORD chunk = static_cast<DWORD>(std::min<uint64_t>(numBytesToRead - totalBytesRead, MAXDWORD));

			DWORD bytesRead = 0;
			if (!::ReadFile(fileHandle.Get(), reinterpret_cast<uint8_t*>(outData) + totalBytesRead, chunk, &bytesRead, nullptr)
				|| bytesRead == 0)
			{
				break;
			}

			totalBytesRead += bytesRead;
		}

		VT_ASSERT(totalBytesRead == static_cast<uint32_t>(numBytesToRead));
	}

	void WindowsPlatformFileSystem::WriteFile(FileHandle fileHandle, const void* data, uint64_t dataSize)
	{
		VT_ASSERT(fileHandle.IsValid());

		DWORD bytesWritten;
		VT_MAYBE_UNUSED bool result = ::WriteFile(fileHandle.Get(), data, static_cast<uint32_t>(dataSize), &bytesWritten, nullptr);

		if (!result)
		{
			DWORD error = GetLastError();
			VT_UNUSED(error);
		}

		VT_ASSERT(result == true && bytesWritten == static_cast<uint32_t>(dataSize));
	}

	uint64_t WindowsPlatformFileSystem::GetFileSize(FileHandle fileHandle)
	{
		VT_ASSERT(fileHandle.IsValid());

		LARGE_INTEGER result;
		GetFileSizeEx(fileHandle.Get(), &result);

		return static_cast<uint64_t>(result.QuadPart);
	}
}

#endif
