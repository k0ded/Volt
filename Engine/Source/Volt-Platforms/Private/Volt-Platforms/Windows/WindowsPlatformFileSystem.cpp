#include "Volt-Platforms/Windows/WindowsPlatformFileSystem.h"

#ifdef VT_PLATFORM_WINDOWS

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/VoltAssert.h>
#include <CoreUtilities/Malloc.h>

#undef CreateFile
#undef CopyFile
#undef CreateDirectory

namespace Volt
{
#define REPARSE_DATA_BUFFER_HEADER_SIZE FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)

	typedef struct REPARSE_DATA_BUFFER
	{
		ULONG ReparseTag;
		USHORT ReparseDataLength;
		USHORT Reserved;

		union
		{
			struct
			{
				USHORT SubstituteNameOffset;
				USHORT SubstituteNameLength;
				USHORT PrintNameOffset;
				USHORT PrintNameLength;
				ULONG Flags;
				WCHAR PathBuffer[1];
			} SymbolicLinkReparseBuffer;

			struct
			{
				USHORT SubstituteNameOffset;
				USHORT SubstituteNameLength;
				USHORT PrintNameOffset;
				USHORT PrintNameLength;
				WCHAR PathBuffer[1];
			} MountPointReparseBuffer;

			struct
			{
				UCHAR DataBuffer[1];
			} GenericReparseBuffer;

		};
	} REPARSE_DATA_BUFFER;

	FileHandle WindowsPlatformFileSystem::OpenFile(const Filesystem::Path& filepath, bool writeable, bool randomAccess)
	{
		HANDLE fileHandle = ::CreateFileW(
			filepath.CStr(),
			writeable ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			randomAccess ? FILE_FLAG_RANDOM_ACCESS : FILE_FLAG_SEQUENTIAL_SCAN,
			nullptr
		);

		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			fileHandle = 0;
		}

		return FileHandle(fileHandle);
	}

	FileHandle WindowsPlatformFileSystem::CreateFile(const Filesystem::Path& destinationFilepath)
	{
		HANDLE fileHandle = ::CreateFileW(
			destinationFilepath.CStr(),
			GENERIC_WRITE,
			FILE_SHARE_READ,
			nullptr,
			CREATE_ALWAYS,
			0,
			nullptr
		);

		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			fileHandle = 0;
		}

		return FileHandle(fileHandle);
	}

	void WindowsPlatformFileSystem::CloseFile(FileHandle fileHandle)
	{
		VT_ASSERT(fileHandle.IsValid());
		CloseHandle(fileHandle.Get());
	}

	void WindowsPlatformFileSystem::CreateDirectory(const Filesystem::Path& path)
	{
		VT_MAYBE_UNUSED BOOL result = CreateDirectoryW(path.CStr(), nullptr);
		VT_ASSERT(result);
	}

	bool WindowsPlatformFileSystem::CopyFile(const Filesystem::Path& from, const Filesystem::Path& to, bool overwriteExisting)
	{
		COPYFILE2_EXTENDED_PARAMETERS params;
		params.dwSize = sizeof(COPYFILE2_EXTENDED_PARAMETERS);
		params.dwCopyFlags = 0;
		params.pfCancel = nullptr;
		params.pProgressRoutine = nullptr;
		params.pvCallbackContext = nullptr;

		if (overwriteExisting)
		{
			params.dwCopyFlags |= COPY_FILE_FAIL_IF_EXISTS;
		}

		HRESULT result = CopyFile2(from.CStr(), to.CStr(), &params);

		if (overwriteExisting &&
			result == ERROR_ALREADY_EXISTS ||
			result == ERROR_FILE_EXISTS)
		{
			return false;
		}

		VT_ASSERT(SUCCEEDED(result));

		return SUCCEEDED(result);
	}

	bool WindowsPlatformFileSystem::CopyDirectory(const Filesystem::Path& from, const Filesystem::Path& to, CopyOptions copyOptions)
	{
		if (!VT_CHECK(IsDirectory(from)))
		{
			return false;
		}

		CreateDirectory(to);

		WindowsPlatformDirectoryIterator directoryIterator(from);

		const bool copyRecursive = (copyOptions & CopyOptions::Recursive) != CopyOptions::None;

		Filesystem::Path path; bool isDirectory;
		while (directoryIterator.Advance(path, isDirectory))
		{
			if (!isDirectory ||
				(copyRecursive && isDirectory))
			{
				Copy(path, from / path.Filename(), copyOptions);
			}
		}

		return true;
	}

	bool WindowsPlatformFileSystem::CopySymlink(const Filesystem::Path& from, const Filesystem::Path& to)
	{
		DWORD attributes = GetFileAttributes(from.CStr());

		HANDLE handle = CreateFileW(
			from.CStr(),
			0,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
			nullptr
		);

		if (handle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		Vector<char> tempBuffer(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
		DWORD bytesReturned = 0;

		BOOL result = DeviceIoControl(
			handle,
			FSCTL_GET_REPARSE_POINT,
			nullptr,
			0,
			tempBuffer.data(),
			static_cast<DWORD>(tempBuffer.size()),
			&bytesReturned,
			nullptr
		);

		CloseHandle(handle);

		if (!result)
		{
			return false;
		}

		REPARSE_DATA_BUFFER* reparseData = reinterpret_cast<REPARSE_DATA_BUFFER*>(tempBuffer.data());

		if (reparseData->ReparseTag != IO_REPARSE_TAG_SYMLINK)
		{
			return false;
		}

		auto& link = reparseData->SymbolicLinkReparseBuffer;

		const size_t pathOffset = link.SubstituteNameOffset / sizeof(WCHAR);
		const size_t pathLength = link.SubstituteNameLength / sizeof(WCHAR);
		WString targetPath(link.PathBuffer + pathOffset, pathLength);
	
		const bool isDirectory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		DWORD flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
		if (isDirectory)
		{
			flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
		}

		if (!CreateSymbolicLinkW(to.CStr(), targetPath.c_str(), flags))
		{
			return false;
		}

		return true;
	}

	void WindowsPlatformFileSystem::Copy(const Filesystem::Path& from, const Filesystem::Path& to, CopyOptions copyOptions)
	{
		DWORD attributes = GetFileAttributes(from.CStr());

		const bool copySymlinks = (copyOptions & CopyOptions::CopySymlinks) != CopyOptions::None;
		const bool copyFiles = (copyOptions & CopyOptions::DirectoriesOnly) == CopyOptions::None;
		const bool skipExisting = (copyOptions & CopyOptions::SkipExisting) != CopyOptions::None;

		if (attributes == INVALID_FILE_ATTRIBUTES)
		{
			return;
		}

		if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
		{
			if (copySymlinks)
			{
				CopySymlink(from, to);
			}
		}
		else if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			Copy(from, to, copyOptions);
		}
		else
		{
			if (copyFiles)
			{
				CopyFile(from, to, skipExisting);
			}
		}
	}

	void WindowsPlatformFileSystem::Rename(const Filesystem::Path& oldPath, const Filesystem::Path& newPath)
	{
		HANDLE handle = CreateFileW(
			oldPath.CStr(),
			DELETE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS,
			nullptr
		);

		if (handle == INVALID_HANDLE_VALUE)
		{
			return;
		}

		size_t size = sizeof(FILE_RENAME_INFO) + (newPath.ToWString().size() * sizeof(wchar_t));

		FILE_RENAME_INFO* info = (FILE_RENAME_INFO*)Memory::Malloc(size);
		memset(info, 0, size);

		info->ReplaceIfExists = TRUE;
		info->FileNameLength = static_cast<DWORD>(newPath.ToWString().size() * sizeof(wchar_t));
		memcpy(info->FileName, newPath.CStr(), info->FileNameLength);

		BOOL result = SetFileInformationByHandle(
			handle,
			FileRenameInfo,
			info,
			static_cast<DWORD>(size)
		);

		VT_ASSERT(result);
		Memory::Free(info);
		CloseHandle(handle);
	}

	Filesystem::Path WindowsPlatformFileSystem::ResolveFinalPathName(FileHandle fileHandle)
	{
		VT_ASSERT(fileHandle.IsValid());

		wchar_t buffer[MAX_PATH];
		DWORD length = GetFinalPathNameByHandleW(
			fileHandle.Get(),
			buffer,
			MAX_PATH,
			FILE_NAME_NORMALIZED
		);

		VT_ASSERT(length > 0);

		WString result(buffer);
		const WString prefix = L"\\\\?\\";

		if (result.find(prefix) == 0)
		{
			result.substr(prefix.size());
		}

		return Filesystem::Path(result);
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

	uint64_t WindowsPlatformFileSystem::GetLastWriteTime(const Filesystem::Path& path)
	{
		FileHandle fileHandle = OpenFile(path, false, false);

		FILETIME ftCreate, ftAccess, ftWrite;
		if (!::GetFileTime(fileHandle.Get(), &ftCreate, &ftAccess, &ftWrite))
		{
			return 0;
		}

		ULARGE_INTEGER temp;
		temp.LowPart = ftWrite.dwLowDateTime;
		temp.HighPart = ftWrite.dwHighDateTime;
	
		constexpr uint64_t WINDOWS_TO_UNIX_MS = 11644473600000ULL;

		return (temp.QuadPart / 10000) - WINDOWS_TO_UNIX_MS;
	}

	void WindowsPlatformFileSystem::MakeWriteable(const Filesystem::Path& path)
	{
		DWORD fileAttribs = ::GetFileAttributes(path.CStr());

		if (fileAttribs == INVALID_FILE_ATTRIBUTES)
		{
			return;
		}

		if ((fileAttribs & FILE_ATTRIBUTE_READONLY) != 0)
		{
			fileAttribs &= ~(FILE_ATTRIBUTE_READONLY);
			::SetFileAttributes(path.CStr(), fileAttribs);
		}
	}

	bool WindowsPlatformFileSystem::Exists(const Filesystem::Path& filepath)
	{
		Filesystem::Path temp = GetWorkingDirectory();

		DWORD fileAttribs = ::GetFileAttributes(filepath.CStr());
		
		if (fileAttribs == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}

		DWORD lastError = GetLastError();

		if (lastError == ERROR_FILE_NOT_FOUND ||
			lastError == ERROR_PATH_NOT_FOUND ||
			lastError == ERROR_INVALID_NAME || 
			lastError == ERROR_BAD_PATHNAME ||
			lastError == ERROR_BAD_NETPATH)
		{
			return false;
		}

		return true;
	}

	bool WindowsPlatformFileSystem::IsWriteable(const Filesystem::Path& filepath)
	{
		DWORD fileAttribs = ::GetFileAttributes(filepath.CStr());
		return fileAttribs != INVALID_FILE_ATTRIBUTES &&
			(fileAttribs & FILE_ATTRIBUTE_READONLY) == 0;
	}

	bool WindowsPlatformFileSystem::IsDirectory(const Filesystem::Path& filepath)
	{
		DWORD fileAttribs = ::GetFileAttributes(filepath.CStr());
		return fileAttribs != INVALID_FILE_ATTRIBUTES &&
			(fileAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	void WindowsPlatformFileSystem::SetWorkingDirectory(const Filesystem::Path& newWorkingDir)
	{
		VT_MAYBE_UNUSED BOOL result = SetCurrentDirectoryW(newWorkingDir.CStr());
		VT_ASSERT(result);
	}

	Filesystem::Path WindowsPlatformFileSystem::GetWorkingDirectory()
	{
		DWORD size = ::GetCurrentDirectory(0, nullptr);

		WString resultStr;
		resultStr.resize(size);

		size = ::GetCurrentDirectory(size, resultStr.data());
		VT_ASSERT(size != 0);
	
		resultStr.resize(size);
		return resultStr;
	}

	Filesystem::Path WindowsPlatformFileSystem::GetExecutableFilepath()
	{
		DWORD bufferSize = MAX_PATH;
		WString resultStr;

		while (true)
		{
			resultStr.resize(bufferSize);
			DWORD size = ::GetModuleFileNameW(nullptr, resultStr.data(), bufferSize);
			VT_ASSERT(size != 0);

			if (size < bufferSize)
			{
				resultStr.resize(size);
				break;
			}

			bufferSize *= 2;
		}

		return resultStr;
	}

	bool WindowsPlatformFileSystem::CreateDirectories(const Filesystem::Path& path)
	{
		if (path.IsEmpty())
		{
			return false;
		}

		const WString& textPath = path.ToWString();

		WString temp;
		temp.reserve(textPath.size());

		const wchar_t* cursor = textPath.begin();
		const wchar_t* end = cursor + textPath.size();

		const wchar_t* rootPathEnd = std::filesystem::_Find_relative_path(cursor, end);
	
		if (rootPathEnd != cursor && 
			end - rootPathEnd >= 3 && 
			std::filesystem::_Is_drive_prefix(rootPathEnd) &&
			std::filesystem::_Is_slash(rootPathEnd[2]))
		{
			// \\?\ prefixes may have a drive letter suffix Windows will reject, strip
			rootPathEnd += 2;
		}

		temp.append(cursor, rootPathEnd);
		cursor = rootPathEnd;

		while (cursor != end)
		{
			const wchar_t* addedEnd = std::find_if(std::find_if_not(cursor, end, std::filesystem::_Is_slash), end, std::filesystem::_Is_slash);
			temp.append(cursor, addedEnd);

			if (!::CreateDirectoryW(temp.c_str(), nullptr))
			{
				DWORD error = GetLastError();
				if (error != ERROR_ALREADY_EXISTS)
				{
					return false;
				}
			}

			cursor = addedEnd;
		}

		return true;
	}

	bool WindowsPlatformFileSystem::Remove(const Filesystem::Path& path)
	{
		DWORD fileAttribs = ::GetFileAttributes(path.CStr());

		if (fileAttribs == INVALID_FILE_ATTRIBUTES ||
			(fileAttribs & FILE_ATTRIBUTE_READONLY) != 0)
		{
			return false;
		}

		BOOL result = false;
		if ((fileAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			if (PathIsDirectoryEmptyW(path.CStr()))
			{
				result = RemoveDirectoryW(path.CStr());
			}
		}
		else
		{
			result = DeleteFileW(path.CStr());
		}

		return result;
	}

	bool WindowsPlatformFileSystem::RemoveAll(const Filesystem::Path& path)
	{
		WIN32_FIND_DATAW data;

		Filesystem::Path searchPath = path / "*";

		HANDLE handle = FindFirstFileW(searchPath.CStr(), &data);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		do
		{
			const wchar_t* filename = data.cFileName;

			if (wcscmp(filename, L".") == 0 || wcscmp(filename, L"..") == 0)
			{
				continue;
			}

			Filesystem::Path child = path / filename;

			const bool isReadOnly = (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
			const bool isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

			if (!isReadOnly)
			{
				if (isDirectory)
				{
					RemoveAll(child);
				}
				else
				{
					DeleteFileW(child.CStr());
				}
			}


			Remove(child);

		} while (FindNextFileW(handle, &data));

		FindClose(handle);

		BOOL result = 0;

		DWORD rootAttribs = ::GetFileAttributes(path.CStr());
		if (PathIsDirectoryEmptyW(path.CStr()) &&
			(rootAttribs & FILE_ATTRIBUTE_READONLY) == 0)
		{
			result = RemoveDirectoryW(path.CStr());
		}

		return result;
	}

	void WindowsPlatformFileSystem::MoveToRecycleBin(const Filesystem::Path& path)
	{
		if (!Exists(path))
		{
			return;
		}

		const WString temp = path.ToWString() + WString(1, L'\0');

		SHFILEOPSTRUCT fileOp;
		fileOp.hwnd = NULL;
		fileOp.wFunc = FO_DELETE;
		fileOp.pFrom = temp.c_str();
		fileOp.pTo = NULL;
		fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOERRORUI | FOF_NOCONFIRMATION | FOF_SILENT;
		SHFileOperation(&fileOp);
	}

	void WindowsPlatformFileSystem::ShowFileInExplorer(const Filesystem::Path& filepath)
	{
		ShellExecuteW(nullptr, L"explore", filepath.CStr(), nullptr, nullptr, SW_SHOWDEFAULT);
	}

	void WindowsPlatformFileSystem::OpenFileExternally(const Filesystem::Path& filepath)
	{
		ShellExecuteW(nullptr, L"open", filepath.CStr(), nullptr, nullptr, SW_SHOWNORMAL);
	}

	WindowsPlatformRecursiveDirectoryIterator::WindowsPlatformRecursiveDirectoryIterator(const Filesystem::Path& rootPath)
	{
		PushDirectory(rootPath);
	}

	WindowsPlatformRecursiveDirectoryIterator::~WindowsPlatformRecursiveDirectoryIterator()
	{
		for (DirectoryState& state : m_stack)
		{
			FindClose(state.handle.Get());
		}
	}

	bool WindowsPlatformRecursiveDirectoryIterator::Advance(Filesystem::Path& outPath, bool& outIsDirectory)
	{
		while (!m_stack.empty())
		{
			DirectoryState& state = m_stack.back();

			uint32_t fileAttribs = state.fileAttributes;
			WString filename = state.filename;

			if (state.first)
			{
				state.first = false;
			}
			else
			{
				WIN32_FIND_DATAW data;
				HANDLE handle = state.handle.Get();

				if (!FindNextFileW(handle, &data))
				{
					FindClose(state.handle.Get());
					m_stack.pop_back();
					continue;
				}

				fileAttribs = data.dwFileAttributes;
				filename = data.cFileName;
			}

			if (filename == L"." || filename == L"..")
			{
				continue;
			}

			outPath = state.path / filename;
			outIsDirectory = (fileAttribs & FILE_ATTRIBUTE_DIRECTORY) != 0;

			if (outIsDirectory)
			{
				PushDirectory(outPath);
			}

			return true;
		}

		return false;
	}

	void WindowsPlatformRecursiveDirectoryIterator::PushDirectory(const Filesystem::Path& path)
	{
		Filesystem::Path searchPath = path / "*";

		WIN32_FIND_DATAW data;
		HANDLE handle = FindFirstFileW(searchPath.CStr(), &data);

		if (handle == INVALID_HANDLE_VALUE)
		{
			return;
		}

		m_stack.emplace_back(path, data.cFileName, FileHandle{ handle }, data.dwFileAttributes, true);
	}

	WindowsPlatformDirectoryIterator::WindowsPlatformDirectoryIterator(const Filesystem::Path& rootPath)
		: m_rootPath(rootPath)
	{
		Filesystem::Path searchPath = rootPath / "*";

		WIN32_FIND_DATAW data;
		HANDLE handle = FindFirstFileW(searchPath.CStr(), &data);

		if (handle == INVALID_HANDLE_VALUE)
		{
			return;
		}

		m_isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		m_activeFilename = data.cFileName;

		m_activeHandle = { handle };
		m_iteration = 0;
	}

	WindowsPlatformDirectoryIterator::~WindowsPlatformDirectoryIterator()
	{
		if (m_activeHandle.IsValid())
		{
			FindClose(m_activeHandle.Get());
		}
	}

	bool WindowsPlatformDirectoryIterator::Advance(Filesystem::Path& outPath, bool& outIsDirectory)
	{
		while (m_activeHandle.IsValid())
		{
			if (m_iteration > 0)
			{
				WIN32_FIND_DATAW data;
				if (!FindNextFileW(m_activeHandle.Get(), &data))
				{
					FindClose(m_activeHandle.Get());
					m_activeHandle.Reset();

					return false;
				}

				m_activeFilename = data.cFileName;
				m_isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			}

			m_iteration++;

			if (m_activeFilename == L"." || m_activeFilename == L"..")
			{
				continue;
			}

			outPath = m_rootPath / m_activeFilename;
			outIsDirectory = m_isDirectory;

			return true;
		}

		return false;
	}

}

#endif
