#include "cupch.h"
#include "CoreUtilities/FileIO/FileUtility.h"
#include "CoreUtilities/FileSystem.h"

#include <fstream>

namespace FileUtility
{
	bool WriteStringToFile(const std::filesystem::path& dstFilepath, std::string_view string, bool createDirectories /*= false*/)
	{
		if (FileSystem::Exists(dstFilepath) && !FileSystem::IsWriteable(dstFilepath))
		{
			return false;
		}

		if (createDirectories)
		{
			if (!FileSystem::Exists(dstFilepath.parent_path()))
			{
				FileSystem::CreateDirectories(dstFilepath.parent_path());
			}
		}

		std::ofstream fileStream;
		fileStream.open(dstFilepath, std::ios::out | std::ios::trunc | std::ios::binary);

		if (!fileStream.is_open() || fileStream.bad())
		{
			return false;
		}

		fileStream.write(string.data(), static_cast<std::streamsize>(string.size()));
		fileStream.close();
		
		return true;
	}

	bool ReadStringFromFile(const std::filesystem::path& srcFilepath, std::string& outString)
	{
		if (!FileSystem::Exists(srcFilepath))
		{
			return false;
		}

		std::ifstream fileStream(srcFilepath, std::ios::in | std::ios::ate | std::ios::binary);

		if (!fileStream.is_open())
		{
			return false;
		}

		const size_t size = fileStream.tellg();
		fileStream.seekg(0);

		outString.resize(size);
		fileStream.read(outString.data(), static_cast<std::streamsize>(size));
		fileStream.close();

		return true;
	}
}
