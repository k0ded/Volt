#include "FileSystemModule/FileUtility.h"
#include "FileSystemModule/FileIORequest.h"
#include "FileSystemModule/IOThreads/IOThreads.h"
#include "FileSystemModule/Filesystem.h"

#include <fstream>

using namespace Volt;

namespace FileUtility
{
	bool WriteStringToFile(const Filesystem::Path& dstFilepath, String&& string, bool createDirectories /*= false*/)
	{
		if (Filesystem::Exists(dstFilepath) && !Filesystem::IsWriteable(dstFilepath))
		{
			return false;
		}

		if (createDirectories)
		{
			if (!Filesystem::Exists(dstFilepath.ParentPath()))
			{
				Filesystem::CreateDirectories(dstFilepath.ParentPath());
			}
		}

		IORequestResult<IORequestWriteFile_String> result = IOThreads::SubmitRequest<IORequestWriteFile_String>("Write File", dstFilepath, std::move(string));
		return result.GetResultCode() == IORequestResultCode::Success;
	}

	bool ReadStringFromFile(const Filesystem::Path& srcFilepath, String& outString)
	{
		if (!Filesystem::Exists(srcFilepath))
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
