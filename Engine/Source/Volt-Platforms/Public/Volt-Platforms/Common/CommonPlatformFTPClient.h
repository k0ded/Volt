#pragma once

#include "Volt-Platforms/Config.h"

#include <CoreUtilities/Filesystem/Path.h>

#include <atomic>

namespace Volt
{
	struct FTPClientConnectInfo
	{
		String username;
		String password;
		String url;
	};

	class VTPL_API CommonPlatformFTPClient
	{
	public:
		CommonPlatformFTPClient();
		~CommonPlatformFTPClient();

		void Connect(const FTPClientConnectInfo& connectInfo);
		void UploadStringAsFile(const Filesystem::Path& targetFilepath, const String& dataStr);

	private:
		void Initialize();
		void Cleanup();


		inline static std::atomic_uint32_t s_numActiveClients = 0;
		inline static std::atomic_bool s_curlInitiailized = false;

		void* m_curlContext = nullptr;
		FTPClientConnectInfo m_connectionInfo;
	};
}
