#include "Volt-FileSystem/FileUtility.h"
#include "Volt-FileSystem/FileIORequest.h"
#include "Volt-FileSystem/IOThreads/IOThreads.h"

#include <CoreUtilities/FileSystem.h>

#include <fstream>

using namespace Volt;

namespace FileUtility
{
	bool WriteStringToFile(const std::filesystem::path& dstFilepath, std::string&& string, bool createDirectories /*= false*/)
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

		IORequestResult<IORequestWriteFile_String> result = IOThreads::SubmitRequest<IORequestWriteFile_String>("Write File", dstFilepath, std::move(string));
		return result.GetResultCode() == IORequestResultCode::Success;
	}

	bool ReadStringFromFile(const std::filesystem::path& srcFilepath, std::string& outString)
	{
		if (!FileSystem::Exists(srcFilepath))
		{
			return false;
		}

		IORequestResult<IORequestReadFile_String> result = IOThreads::SubmitRequest<IORequestReadFile_String>("Read File", srcFilepath);

		if (result.GetResultCode() == IORequestResultCode::Failure)
		{
			return false;
		}

		outString = result.GetResult();
		return true;
	}
}
