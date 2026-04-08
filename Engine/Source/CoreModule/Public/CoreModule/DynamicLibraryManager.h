#pragma once

#include "CoreModule/Config.h"

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Filesystem/Path.h>

namespace Volt
{
	typedef void* DLLHandle;

	class DynamicLibraryManager : public SubSystem
	{
	public:
		VTC_API DynamicLibraryManager();
		VTC_API ~DynamicLibraryManager();

		VT_NODISCARD VTC_API DLLHandle LoadDynamicLibrary(const Filesystem::Path& binaryFilepath, bool& outExternallyLoaded);
		VTC_API bool UnloadDynamicLibrary(const Filesystem::Path& binaryFilepath);

		VTC_API static DynamicLibraryManager& Get();

		VT_DECLARE_SUBSYSTEM("{FCB778C9-2E7F-4E47-AFC7-A6186AC89ACE}"_guid)

	private:
		inline static DynamicLibraryManager* s_instance = nullptr;

		Map<Filesystem::Path, DLLHandle> m_loadedDynamicLibraries;
	};
}
