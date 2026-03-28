#pragma once

#include "FileSystemModule/Config.h"

#include <PlatformsModule/FileSystem.h>

#include <CoreUtilities/Filesystem/Path.h>
#include <CoreUtilities/Containers/Vector.h>



namespace Volt
{
	class CommandLineBuilder;
}

namespace Filesystem
{
	extern VTFS_API void MakeWriteable(const Filesystem::Path& path);
	extern VTFS_API bool IsWriteable(const Filesystem::Path& path);
	extern VTFS_API bool Exists(const Filesystem::Path& path);
	extern VTFS_API bool FilepathIsOnlyExtension(const Filesystem::Path& path);
	extern VTFS_API bool IsFilepathInDirectory(const Filesystem::Path& directoryPath, const Filesystem::Path& filepath, bool checkSubDirectories = false);

	extern VTFS_API bool CreateDirectories(const Filesystem::Path& path);

	extern VTFS_API uint64_t GetLastWriteTime(const Filesystem::Path& path);
	extern VTFS_API uint64_t GetFileSize(const Filesystem::Path& filepath);

	extern VTFS_API void SetWorkingDirectory(const Filesystem::Path& newWorkingDir);
	extern VTFS_API Filesystem::Path GetWorkingDirectory();
	extern VTFS_API Filesystem::Path GetExecutablePath();

	extern VTFS_API Filesystem::Path Relative(const Filesystem::Path& path, const Filesystem::Path& base = GetWorkingDirectory());
	extern VTFS_API Filesystem::Path Proximate(const Filesystem::Path& path, const Filesystem::Path& base = GetWorkingDirectory());
	extern VTFS_API Filesystem::Path Absolute(const Filesystem::Path& path);
	extern VTFS_API Filesystem::Path Cannonical(const Filesystem::Path& filepath);
	extern VTFS_API Filesystem::Path WeaklyCannonical(const Filesystem::Path& filepath);

	extern VTFS_API void Copy(const Filesystem::Path& from, const Filesystem::Path& to, CopyOptions copyOptions = CopyOptions::None);
	extern VTFS_API void CopyFile(const Filesystem::Path& from, const Filesystem::Path& to, bool overwriteExisting = true);
	
	extern VTFS_API void Rename(const Filesystem::Path& oldPath, const Filesystem::Path& newPath);
	extern VTFS_API void MoveTo(const Filesystem::Path& path, const Filesystem::Path& destinationDir);
	extern VTFS_API void Move(const Filesystem::Path& sourcePath, const Filesystem::Path& destinationPath);

	extern VTFS_API bool Remove(const Filesystem::Path& path);
	extern VTFS_API bool RemoveAll(const Filesystem::Path& path);
	extern VTFS_API void MoveToRecycleBin(const Filesystem::Path& path);

	extern VTFS_API void InitializeWorkingDirectory(bool isRuntime, const Filesystem::Path& workingDirectory, const Filesystem::Path& executableFilepath);

	extern VTFS_API bool ShowFileInExplorer(const Filesystem::Path& filepath);
	extern VTFS_API bool OpenFileExternally(const Filesystem::Path& filepath);

#if 0
	extern VTFS_API void MakeWriteable(const Filesystem::Path& path);
	extern VTFS_API bool Copy(const Filesystem::Path& source, const Filesystem::Path& destination);
	extern VTFS_API bool CopyFileToDirectory(const Filesystem::Path& source, const Filesystem::Path& dstDir);
	extern VTFS_API bool Remove(const Filesystem::Path& path);
	extern VTFS_API bool Rename(const Filesystem::Path& filepath, const String& name);
	extern VTFS_API bool Move(const Filesystem::Path& filepath, const Filesystem::Path& dstDir);
	extern VTFS_API bool MoveDirectory(const Filesystem::Path& srcDir, const Filesystem::Path& dstDir);
	extern VTFS_API bool IsFilepathInDirectory(const Filesystem::Path& directoryPath, const Filesystem::Path& filepath, bool checkSubDirectories = false);

	extern VTFS_API bool ShowDirectoryInExplorer(const Filesystem::Path& dir);
	extern VTFS_API bool OpenFileExternally(const Filesystem::Path& filepath);
	extern VTFS_API void MoveToRecycleBin(const Filesystem::Path& path);

	extern VTFS_API Filesystem::Path GetDocumentsPath();
	extern VTFS_API Filesystem::Path GetExecutablePath();

#endif
}
