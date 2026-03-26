#pragma once

#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Config.h"
#include "Volt-Platforms/FileHandle.h"
#include "Volt-Platforms/FileSystem.h"

#include <CoreUtilities/Containers/ArrayView.h>
#include <CoreUtilities/Filesystem/Path.h>

#include <filesystem>

namespace Volt
{
	class VTPL_API WindowsPlatformFileSystem
	{
	public:
		static FileHandle OpenFile(const Filesystem::Path& filepath, bool writeable, bool randomAccess);
		static FileHandle CreateFile(const Filesystem::Path& destinationFilepath);
		static void CloseFile(FileHandle fileHandle);

		static void ReadFile(FileHandle fileHandle, uint64_t numBytesToRead, void* outData, uint64_t outDataSize);
		static void WriteFile(FileHandle fileHandle, const void* data, uint64_t dataSize);
		static uint64_t GetFileSize(FileHandle fileHandle);

		static void CreateDirectory(const Filesystem::Path& path);

		static bool CopyFile(const Filesystem::Path& from, const Filesystem::Path& to, bool overwriteExisting = true);
		static bool CopyDirectory(const Filesystem::Path& from, const Filesystem::Path& to, CopyOptions copyOptions = CopyOptions::None);
		static bool CopySymlink(const Filesystem::Path& from, const Filesystem::Path& to);
		static void Copy(const Filesystem::Path& from, const Filesystem::Path& to, CopyOptions copyOptions = CopyOptions::None);

		static void Rename(const Filesystem::Path& oldPath, const Filesystem::Path& newPath);

		static Filesystem::Path ResolveFinalPathName(FileHandle fileHandle);

		static uint64_t GetLastWriteTime(const Filesystem::Path& path);

		static void MakeWriteable(const Filesystem::Path& path);
		static bool IsWriteable(const Filesystem::Path& filepath);
		static bool Exists(const Filesystem::Path& filepath);
		static bool IsDirectory(const Filesystem::Path& filepath);

		static void SetWorkingDirectory(const Filesystem::Path& newWorkingDir);
		static Filesystem::Path GetWorkingDirectory();
		static Filesystem::Path GetExecutableFilepath();

		static bool CreateDirectories(const Filesystem::Path& path);

		static bool Remove(const Filesystem::Path& path);
		static bool RemoveAll(const Filesystem::Path& path);
		static void MoveToRecycleBin(const Filesystem::Path& path);

		static void ShowFileInExplorer(const Filesystem::Path& filepath);
		static void OpenFileExternally(const Filesystem::Path& filepath);
	};

	class WindowsPlatformRecursiveDirectoryIterator
	{
	public:
		WindowsPlatformRecursiveDirectoryIterator() = default;
		VTPL_API ~WindowsPlatformRecursiveDirectoryIterator();

		VTPL_API WindowsPlatformRecursiveDirectoryIterator(const Filesystem::Path& rootPath);

		VTPL_API bool Advance(Filesystem::Path& outPath, bool& outIsDirectory);

		VT_INLINE bool operator!=(const WindowsPlatformRecursiveDirectoryIterator& other) const { return m_stack.size() != other.m_stack.size(); }

	private:
		VTPL_API void PushDirectory(const Filesystem::Path& path);

		struct DirectoryState
		{
			Filesystem::Path path;
			WString filename;
			FileHandle handle;
			uint32_t fileAttributes;
			bool first;
		};

		Vector<DirectoryState> m_stack;
	};

	class WindowsPlatformDirectoryIterator
	{
	public:
		WindowsPlatformDirectoryIterator() = default;
		VTPL_API ~WindowsPlatformDirectoryIterator();

		VTPL_API WindowsPlatformDirectoryIterator(const Filesystem::Path& rootPath);

		VTPL_API bool Advance(Filesystem::Path& outPath, bool& outIsDirectory);
		VT_INLINE bool operator!=(const WindowsPlatformDirectoryIterator& other) const { return m_iteration != other.m_iteration; }

	public:
		Filesystem::Path m_rootPath;
		WString m_activeFilename;
		FileHandle m_activeHandle;
		uint32_t m_iteration = 0xFFFFFFFF;

		bool m_isDirectory = false;
	};
}

#endif
