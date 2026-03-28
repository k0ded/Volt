
#include "FileSystemModule/Filesystem.h"

#include <Volt-Platforms/Platform.h>

#include <CoreUtilities/String/StringUtility.h>

namespace Filesystem
{
	void MakeWriteable(const Path& path)
	{
		Volt::PlatformFileSystem::MakeWriteable(path);
	}

	bool IsWriteable(const Path& path)
	{
		return Volt::PlatformFileSystem::IsWriteable(path);
	}

	bool Exists(const Path& path)
	{
		return Volt::PlatformFileSystem::Exists(path);
	}

	bool FilepathIsOnlyExtension(const Path& path)
	{
		const String filename = path.Filename().ToString();

		if (!filename.empty())
		{
			return filename[0] == '.' && filename.find_first_of('.', 1) == String::npos;
		}

		return false;
	}

	bool IsFilepathInDirectory(const Path& directoryPath, const Path& filepath, bool checkSubDirectories /*= false*/)
	{
		Filesystem::Path canonicalFilepath = WeaklyCannonical(filepath);
		Filesystem::Path canonicalDirectoryPath = WeaklyCannonical(directoryPath);

		if (canonicalFilepath == canonicalDirectoryPath)
		{
			return true;
		}

		if (checkSubDirectories)
		{
			return std::mismatch(canonicalDirectoryPath.begin(), canonicalDirectoryPath.end(), canonicalFilepath.begin()).first == canonicalDirectoryPath.end();
		}

		return canonicalFilepath.ParentPath() == canonicalDirectoryPath;
	}

	bool CreateDirectories(const Path& path)
	{
		return Volt::PlatformFileSystem::CreateDirectories(path);
	}

	uint64_t GetLastWriteTime(const Path& path)
	{
		return Volt::PlatformFileSystem::GetLastWriteTime(path);
	}

	uint64_t GetFileSize(const Path& filepath)
	{
		Volt::FileHandle handle = Volt::PlatformFileSystem::OpenFile(filepath, false, false);
		uint64_t size = Volt::PlatformFileSystem::GetFileSize(handle);
		Volt::PlatformFileSystem::CloseFile(handle);

		return size;
	}

	void SetWorkingDirectory(const Path& newWorkingDir)
	{
		Volt::PlatformFileSystem::SetWorkingDirectory(newWorkingDir);
	}

	Filesystem::Path GetWorkingDirectory()
	{
		return Volt::PlatformFileSystem::GetWorkingDirectory();
	}

	Filesystem::Path GetExecutablePath()
	{
		return Volt::PlatformFileSystem::GetExecutableFilepath();
	}

	Filesystem::Path Relative(const Path& path, const Path& base)
	{
		std::filesystem::path tempPath(path.ToWString().begin(), path.ToWString().end());
		std::filesystem::path tempBase(base.ToWString().begin(), base.ToWString().end());
		std::wstring relative = std::filesystem::relative(tempPath, tempBase).wstring();

		return Path(WStringView(relative.c_str(), relative.size()));
	}

	Filesystem::Path Proximate(const Path& path, const Path& base)
	{
		std::filesystem::path tempPath(path.ToWString().begin(), path.ToWString().end());
		std::filesystem::path tempBase(base.ToWString().begin(), base.ToWString().end());
		std::wstring proximate = std::filesystem::proximate(tempPath, tempBase).wstring();

		return Path(WStringView(proximate.c_str(), proximate.size()));
	}

	Filesystem::Path Absolute(const Path& path)
	{
		std::filesystem::path tempPath(path.ToWString().begin(), path.ToWString().end());
		std::wstring absolute = std::filesystem::absolute(tempPath).wstring();

		return Path(WStringView(absolute.c_str(), absolute.size()));
	}

	Filesystem::Path Cannonical(const Path& filepath)
	{
		Path absolute = Absolute(filepath);

		VT_ASSERT(Exists(absolute));

		Volt::FileHandle handle = Volt::PlatformFileSystem::OpenFile(absolute, false, false);
		VT_ASSERT(handle.IsValid());

		const Path cannonical = Volt::PlatformFileSystem::ResolveFinalPathName(handle);
		Volt::PlatformFileSystem::CloseFile(handle);

		return cannonical;
	}

	Filesystem::Path WeaklyCannonical(const Path& filepath)
	{
		Path absolute = Absolute(filepath);

		Vector<WString> parts = Utility::SplitStringsByCharacter(absolute.ToWString(), L'\\');

		Path current;
		size_t i = 0;

		for (; i < parts.size(); ++i)
		{
			Path test = current / parts[i];

			if (!Exists(test))
			{
				break;
			}

			current = test;
		}

		Path result = current;

		if (Exists(current))
		{
			result = Cannonical(current);
		}

		for (; i < parts.size(); ++i)
		{
			result /= parts[i];
		}

		return result.LexicallyNormal();
	}

	void Copy(const Path& from, const Path& to, CopyOptions copyOptions /*= CopyOptions::None*/)
	{
		Volt::PlatformFileSystem::Copy(from, to, copyOptions);
	}

	void CopyFile(const Path& from, const Path& to, bool overwriteExisting /*= true*/)
	{
		Volt::PlatformFileSystem::CopyFile(from, to, overwriteExisting);
	}

	void Rename(const Path& oldPath, const Path& newPath)
	{
		Volt::PlatformFileSystem::Rename(oldPath, newPath);
	}

	void MoveTo(const Path& path, const Path& destinationDir)
	{
		if (!Exists(destinationDir))
		{
			return;
		}

		Rename(path, destinationDir / path.Filename());
	}

	void Move(const Path& sourcePath, const Path& destinationPath)
	{
		Volt::PlatformFileSystem::Rename(sourcePath, destinationPath);
	}

	bool Remove(const Path& path)
	{
		return Volt::WindowsPlatformFileSystem::Remove(path);
	}

	bool RemoveAll(const Path& path)
	{
		return Volt::WindowsPlatformFileSystem::RemoveAll(path);
	}

	void MoveToRecycleBin(const Path& path)
	{
		Volt::WindowsPlatformFileSystem::MoveToRecycleBin(path);
	}

	void InitializeWorkingDirectory(bool isRuntime, const Filesystem::Path& workingDirectory, const Filesystem::Path& executableFilepath)
	{
		// We don't need to do anything if we are in the "runtime" aka game launcher.
		if (!isRuntime)
		{
			// Check if we have an override for the working directory
			if (!workingDirectory.IsEmpty())
			{
				Filesystem::Path filepath = Filesystem::Absolute(workingDirectory);
				if (Filesystem::Exists(filepath))
				{
					Filesystem::SetWorkingDirectory(filepath);
					return;
				}
			}

			const Filesystem::Path currentDirectory = Filesystem::GetWorkingDirectory();

			// Make sure that the working directory isn't already correct with some hopefully correct checks
			if (currentDirectory.Stem() == "Engine" && Filesystem::Exists(currentDirectory / "Binaries"))
			{
				return;
			}

			Filesystem::Path binariesFilepath = executableFilepath.ParentPath();

			// We assume that we currently are in the "Binaries" directory
			VT_ASSERT(binariesFilepath.Stem() == "Binaries");

			// Then our new working directory is just our parent directory, which hopefully is correct.
			Filesystem::SetWorkingDirectory(binariesFilepath.ParentPath());
		}
	}

	bool ShowFileInExplorer(const Path& filepath)
	{
		auto absolutePath = Cannonical(filepath);
		if (!Exists(absolutePath))
		{
			return false;
		}

		Volt::PlatformFileSystem::ShowFileInExplorer(filepath);
		return true;
	}

	bool OpenFileExternally(const Path& filepath)
	{
		auto absolutePath = Cannonical(filepath);
		if (!Exists(absolutePath))
		{
			return false;
		}

		Volt::PlatformFileSystem::OpenFileExternally(filepath);
		return true;
	}

#if 0
	bool IsWriteable(const Filesystem::Path& path)
	{
		std::filesystem::file_status status = std::filesystem::status(path);
		return (status.permissions() & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
	}

	void MakeWriteable(const Filesystem::Path& path)
	{
		std::filesystem::permissions(path, std::filesystem::perms::owner_write, std::filesystem::perm_options::add);
	}

	bool Copy(const Filesystem::Path& source, const Filesystem::Path& destination)
	{
		if (!Exists(source))
		{
			return false;
		}

		std::filesystem::copy(source, destination);
		return true;
	}

	bool CopyFileToDirectory(const Filesystem::Path& source, const Filesystem::Path& dstDir)
	{
		if (!Exists(dstDir))
		{
			return false;
		}

		Filesystem::Path newPath = dstDir / source.filename();
		Copy(source, newPath);
		return true;
	}

	bool Exists(const Filesystem::Path& path)
	{
		return std::filesystem::exists(path);
	}

	bool Remove(const Filesystem::Path& path)
	{
		return std::filesystem::remove_all(path);
	}

	bool Rename(const Filesystem::Path& filepath, const String& name)
	{
		if (!Exists(filepath))
		{
			return false;
		}

		const Filesystem::Path newPath = filepath.parent_path() / (name + filepath.extension().string());
		std::filesystem::rename(filepath, newPath);

		return true;
	}

	bool Move(const Filesystem::Path& filepath, const Filesystem::Path& dstDir)
	{
		if (!Exists(filepath))
		{
			return false;
		}

		const Filesystem::Path newPath = dstDir / filepath.filename();
		std::filesystem::rename(filepath, newPath);

		return true;
	}

	bool MoveDirectory(const Filesystem::Path& srcDir, const Filesystem::Path& dstDir)
	{
		if (!Exists(srcDir))
		{
			return false;
		}

		std::filesystem::rename(srcDir, dstDir);
		return true;
	}

	bool CreateDirectories(const Filesystem::Path& path)
	{
		if (Exists(path))
		{
			return true;
		}

		return std::filesystem::create_directories(path);
	}

	bool FilePathIsOnlyExtension(const Filesystem::Path& path)
	{
		const String filename = path.filename().string();

		if (!filename.empty())
		{
			return filename[0] == '.' && filename.find_first_of('.', 1) == String::npos;
		}

		return false;
	}

	bool IsFilepathInDirectory(const Filesystem::Path& directoryPath, const Filesystem::Path& filepath, bool checkSubDirectories /*= false*/)
	{
		Filesystem::Path canonicalFilepath = std::filesystem::weakly_canonical(filepath);
		Filesystem::Path canonicalDirectoryPath = std::filesystem::weakly_canonical(directoryPath);

		if (canonicalFilepath == canonicalDirectoryPath)
		{
			return true;
		}

		if (checkSubDirectories)
		{
			return std::mismatch(canonicalDirectoryPath.begin(), canonicalDirectoryPath.end(), canonicalFilepath.begin()).first == canonicalDirectoryPath.end();
		}

		return canonicalFilepath.parent_path() == canonicalDirectoryPath;
	}

	bool ShowDirectoryInExplorer(const Filesystem::Path& dir)
	{
		auto absolutePath = std::filesystem::canonical(dir);
		if (!std::filesystem::exists(absolutePath))
		{
			return false;
		}

		bool succeded = false;

		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(hr))
		{
			PIDLIST_ABSOLUTE pidl = ILCreateFromPath(absolutePath.c_str());
			if (pidl)
			{
				hr = SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);

				succeded = SUCCEEDED(hr);

				ILFree(pidl);
			}
		}

		CoUninitialize();
		return succeded;
	}

	bool ShowFileInExplorer(const Filesystem::Path& filepath)
	{
		auto absolutePath = std::filesystem::canonical(filepath);
		if (!Exists(absolutePath))
		{
			return false;
		}

		ShellExecute(NULL, L"explore", absolutePath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
		return true;
	}

	bool OpenFileExternally(const Filesystem::Path& filepath)
	{

	}

	void MoveToRecycleBin(const Filesystem::Path& path)
	{
		const auto canonicalPath = std::filesystem::canonical(path);

		if (!std::filesystem::exists(canonicalPath))
		{
			return;
		}

		std::wstring wstr = canonicalPath.wstring() + std::wstring(1, L'\0');

		SHFILEOPSTRUCT fileOp;
		fileOp.hwnd = NULL;
		fileOp.wFunc = FO_DELETE;
		fileOp.pFrom = wstr.c_str();
		fileOp.pTo = NULL;
		fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOERRORUI | FOF_NOCONFIRMATION | FOF_SILENT;
		SHFileOperation(&fileOp);
	}

	Filesystem::Path GetDocumentsPath()
	{
		TCHAR* path = 0;
		Filesystem::Path documentsPath;
		SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_SIMPLE_IDLIST, NULL, &path);
		documentsPath = path;
		CoTaskMemFree(path);

		return documentsPath;
	}

	Filesystem::Path GetExecutablePath()
	{
		constexpr uint64_t MAX_LENGTH = 255;
		char filename[MAX_LENGTH];
		::GetModuleFileNameA(nullptr, filename, MAX_LENGTH);

		return Filesystem::Path(filename);
	}

	bool HasEnvironmentVariable(const String& key)
	{
		HKEY hKey;
		LPCSTR keyPath = "Environment";
		LSTATUS lOpenStatus = RegOpenKeyExA(HKEY_CURRENT_USER, keyPath, 0, KEY_ALL_ACCESS, &hKey);

		if (lOpenStatus == ERROR_SUCCESS)
		{
			lOpenStatus = RegQueryValueExA(hKey, key.c_str(), nullptr, nullptr, nullptr, nullptr);
			RegCloseKey(hKey);
		}

		return lOpenStatus == ERROR_SUCCESS;
	}

	bool SetEnvironmentVariable(const String& key, const String& value)
	{
		HKEY hKey;
		LPCSTR keyPath = "Environment";
		DWORD createdNewKey;
		LSTATUS lOpenStatus = RegCreateKeyExA(HKEY_CURRENT_USER, keyPath, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, &createdNewKey);
		if (lOpenStatus == ERROR_SUCCESS)
		{
			LSTATUS lSetStatus = RegSetValueExA(hKey, key.c_str(), 0, REG_SZ, (LPBYTE)value.c_str(), (DWORD)value.length() + 1);
			RegCloseKey(hKey);

			if (lSetStatus == ERROR_SUCCESS)
			{
				SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_BLOCK, 100, nullptr);
				return true;
			}
		}

		return false;
	}

	bool SetRegistryValue(const String& key, const String& value)
	{
		HKEY hkey;

		char valueCurrent[1000];
		DWORD type;
		DWORD size = sizeof(valueCurrent);

		int rc = RegGetValueA(HKEY_CURRENT_USER, key.c_str(), nullptr, RRF_RT_ANY, &type, valueCurrent, &size);

		bool notFound = rc == ERROR_FILE_NOT_FOUND;

		if (rc != ERROR_SUCCESS && !notFound)
		{
			// Error ?
		}

		if (!notFound)
		{
			if (type != REG_SZ)
			{
				// Error ?
			}

			if (strcmp(valueCurrent, value.c_str()) == 0)
			{
				return true;
			}
		}

		DWORD disposition;
		rc = RegCreateKeyExA(HKEY_CURRENT_USER, key.c_str(), 0, 0, 0, KEY_ALL_ACCESS, nullptr, &hkey, &disposition);
		if (rc != ERROR_SUCCESS)
		{
			return false;
		}

		rc = RegSetValueExA(hkey, "", 0, REG_SZ, (BYTE*)value.c_str(), (DWORD)strlen(value.c_str()) + 1);
		if (rc != ERROR_SUCCESS)
		{
			return false;
		}

		RegCloseKey(hkey);

		return true;
	}

	String GetEnvironmentVariableValue(const String& key)
	{
		HKEY hKey;
		LPCSTR keyPath = "Environment";
		DWORD createdNewKey;
		LSTATUS lOpenStatus = RegCreateKeyExA(HKEY_CURRENT_USER, keyPath, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, &createdNewKey);
		if (lOpenStatus == ERROR_SUCCESS)
		{
			DWORD valueType;
			auto* data = new char[512];
			DWORD dataSize = 512;
			LSTATUS status = RegGetValueA(hKey, nullptr, key.c_str(), RRF_RT_ANY, &valueType, (PVOID)data, &dataSize);

			RegCloseKey(hKey);

			if (status == ERROR_SUCCESS)
			{
				String result(data);
				delete[] data;
				return result;
			}

			delete[] data;
		}

		return String{};
	}

	String GetCurrentUserName()
	{
		String username;

		TCHAR name[UNLEN + 1];
		DWORD size = UNLEN + 1;

		GetUserName(name, &size);
		return String(String::CtorConvert(), name, size);
	}

	void StartProcess(const Filesystem::Path& processPath)
	{
		std::wstring processDir = processPath.parent_path().wstring();
		std::wstring tempProcessName = processPath.wstring();
		tempProcessName.insert(tempProcessName.begin(), '\"');
		tempProcessName.push_back('\"');

		SHELLEXECUTEINFO ShExecInfo = {};
		ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		ShExecInfo.hwnd = NULL;
		ShExecInfo.lpVerb = L"open";
		ShExecInfo.lpFile = tempProcessName.c_str();
		ShExecInfo.lpParameters = L"";
		ShExecInfo.lpDirectory = processDir.c_str();
		ShExecInfo.nShow = SW_SHOW;
		ShExecInfo.hInstApp = NULL;
		ShellExecuteEx(&ShExecInfo);
	}

	bool RunCommand(const String& aCommand)
	{
		return system(aCommand.c_str());
	}
#endif

}
