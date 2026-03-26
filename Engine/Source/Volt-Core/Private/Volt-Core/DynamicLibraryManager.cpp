#include "vtcorepch.h"
#include "Volt-Core/DynamicLibraryManager.h"

#include <CoreUtilities/DynamicLibraryHelpers.h>
#include <CoreUtilities/String/StringUtility.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(DynamicLibraryManager, Default, PreEngine);

	DynamicLibraryManager::DynamicLibraryManager()
	{
		VT_ASSERT(s_instance == nullptr);
		s_instance = this;
	}

	DynamicLibraryManager::~DynamicLibraryManager()
	{
		VT_ENSURE(m_loadedDynamicLibraries.empty());
		s_instance = nullptr;
	}

	VT_NODISCARD DLLHandle DynamicLibraryManager::LoadDynamicLibrary(const Filesystem::Path& binaryFilepath, bool& outExternallyLoaded)
	{
		Filesystem::Path tempPath = binaryFilepath;
		tempPath.MakePreferred();

		DLLHandle handle = VT_GET_MODULE_HANDLE(tempPath.ToWString().c_str());
		outExternallyLoaded = handle != nullptr;

		if (!outExternallyLoaded)
		{
			handle = VT_LOAD_LIBRARY(tempPath.CStr());
		}

		if (!handle)
		{
			VT_LOGC(Error, LogApplication, "Unable to load DLL {}!", tempPath);
			return nullptr;
		}

		m_loadedDynamicLibraries[tempPath] = handle;
		return handle;
	}

	bool DynamicLibraryManager::UnloadDynamicLibrary(const Filesystem::Path& binaryFilepath)
	{
		Filesystem::Path tempPath = binaryFilepath;
		tempPath.MakePreferred();

		if (!m_loadedDynamicLibraries.contains(tempPath))
		{
			return false;
		}

		VT_FREE_LIBRARY(m_loadedDynamicLibraries.at(tempPath));
		m_loadedDynamicLibraries.erase(tempPath);
	
		return true;
	}
}
