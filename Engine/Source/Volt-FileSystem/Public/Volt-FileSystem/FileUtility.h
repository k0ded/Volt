#pragma once

#include "Volt-FileSystem/Config.h"

#include <filesystem>

namespace FileUtility
{
	VTFS_API bool WriteStringToFile(const std::filesystem::path& dstFilepath, std::string&& string, bool createDirectories = false);
	VTFS_API bool ReadStringFromFile(const std::filesystem::path& srcFilepath, std::string& outString);
}
