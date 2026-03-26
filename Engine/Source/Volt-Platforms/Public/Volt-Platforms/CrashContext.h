#pragma once

#include <CoreUtilities/Archive/Archive.h>

#include <cstdint>

namespace Volt
{
	struct CrashContext
	{
		void* platformCrashContext;
		uint32_t crashingThreadId;
		String stackTrace;
		String username;
		String timestamp;
		String commandLine;
		String errorString;

		String serverURL;
		String serverUser;
		String serverPassword;
		
		friend Archive& operator<<(Archive& archive, CrashContext& value)
		{
			uint64_t ptrVal = std::bit_cast<uint64_t>(value.platformCrashContext);

			archive << ptrVal;

			if (archive.IsLoading())
			{
				value.platformCrashContext = std::bit_cast<void*>(ptrVal);
			}

			archive << value.crashingThreadId;
			archive << value.stackTrace;
			archive << value.username;
			archive << value.timestamp;
			archive << value.commandLine;
			archive << value.errorString;
			archive << value.serverURL;
			archive << value.serverUser;
			archive << value.serverPassword;

			return archive;
		}
	};
}
