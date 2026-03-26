#pragma once

#include "Volt-Core/Config.h"

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Filesystem/Path.h>

namespace Volt
{
	typedef void* DLLHandle;

	class VTCORE_API DynamicLibraryManager : public SubSystem
	{
	public:
		DynamicLibraryManager();
		~DynamicLibraryManager();

		VT_NODISCARD DLLHandle LoadDynamicLibrary(const Filesystem::Path& binaryFilepath, bool& outExternallyLoaded);
		bool UnloadDynamicLibrary(const Filesystem::Path& binaryFilepath);

		VT_NODISCARD VT_INLINE static DynamicLibraryManager& Get() { return *s_instance; }

		VT_DECLARE_SUBSYSTEM("{FCB778C9-2E7F-4E47-AFC7-A6186AC89ACE}"_guid)

	private:
		inline static DynamicLibraryManager* s_instance = nullptr;

		Map<Filesystem::Path, DLLHandle> m_loadedDynamicLibraries;
	};
}
