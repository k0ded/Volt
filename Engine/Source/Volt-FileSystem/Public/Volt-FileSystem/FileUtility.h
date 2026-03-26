#pragma once

#include "Volt-FileSystem/Config.h"

#include <CoreUtilities/Filesystem/Path.h>

namespace FileUtility
{
	VTFS_API bool WriteStringToFile(const Filesystem::Path& dstFilepath, String&& string, bool createDirectories = false);
	VTFS_API bool ReadStringFromFile(const Filesystem::Path& srcFilepath, String& outString);
}
