#pragma once

#include "CoreUtilities/Config.h"

#include <filesystem>

namespace FileUtility
{
	VTCOREUTIL_API bool WriteStringToFile(const std::filesystem::path& dstFilepath, std::string_view string, bool createDirectories = false);
	VTCOREUTIL_API bool ReadStringFromFile(const std::filesystem::path& srcFilepath, std::string& outString);
}
