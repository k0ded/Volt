#pragma once

#include "Volt-Platforms/Config.h"

#include <string>
#include <atomic>
#include <filesystem>

namespace Volt
{
	struct FTPClientConnectInfo
	{
		std::string username;
		std::string password;
		std::string url;
	};

	class VTPL_API CommonPlatformFTPClient
	{
	public:
		CommonPlatformFTPClient();
		~CommonPlatformFTPClient();

		void Connect(const FTPClientConnectInfo& connectInfo);
		void UploadStringAsFile(const std::filesystem::path& targetFilepath, const std::string& dataStr);

	private:
		void Initialize();
		void Cleanup();


		inline static std::atomic_uint32_t s_numActiveClients = 0;
		inline static std::atomic_bool s_curlInitiailized = false;

		void* m_curlContext = nullptr;
		FTPClientConnectInfo m_connectionInfo;
	};
}
