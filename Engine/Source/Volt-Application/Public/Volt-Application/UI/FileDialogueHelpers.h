#pragma once

#include "Volt-Application/Config.h"

#include <CoreUtilities/Filesystem/Path.h>

namespace FileDialogueHelpers
{
	struct FileFilter
	{
		String name;
		String extensions;
	};

	extern VTAPP_API void Initialize();
	extern VTAPP_API void Shutdown();

	extern VTAPP_API Filesystem::Path PickFolderDialogue(const Filesystem::Path& baseDir);
	extern VTAPP_API Filesystem::Path OpenFileDialogue(const Vector<FileFilter>& filters, const Filesystem::Path& baseDir);
	extern VTAPP_API Filesystem::Path SaveFileDialogue(const Vector<FileFilter>& filters, const Filesystem::Path& baseDir);
}
