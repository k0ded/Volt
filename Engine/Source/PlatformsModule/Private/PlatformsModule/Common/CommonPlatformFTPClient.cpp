#include "PlatformsModule/Common/CommonPlatformFTPClient.h"

#include <LogModule/Log.h>

#include <curl/curl.h>
#include <CoreUtilities/VoltAssert.h>

namespace Volt
{
	struct UploadData
	{
		const uint8_t* data;
		size_t size;
		size_t position;
	};

	inline size_t DataReadCallback(char* buffer, size_t size, size_t nitems, void* userdata)
	{
		UploadData* uploadData = static_cast<UploadData*>(userdata);

		const size_t bufferSize = size * nitems;
		const size_t remaining = uploadData->size - uploadData->position;
		const size_t toCopy = (remaining < bufferSize) ? remaining : bufferSize;

		if (toCopy > 0)
		{
			memcpy(buffer, uploadData->data + uploadData->position, toCopy);
			uploadData->position += toCopy;
			return toCopy;
		}

		return 0;
	}

	CommonPlatformFTPClient::CommonPlatformFTPClient()
	{
		Initialize();
	}

	CommonPlatformFTPClient::~CommonPlatformFTPClient()
	{
		Cleanup();
	}

	void CommonPlatformFTPClient::Connect(const FTPClientConnectInfo& connectInfo)
	{
		m_connectionInfo = connectInfo;

		// Make sure previous context is removed.
		if (m_curlContext)
		{
			curl_easy_cleanup(m_curlContext);
			m_curlContext = nullptr;
		}

		m_curlContext = curl_easy_init();

		// "Connect"
		const String usrPwdString = connectInfo.username + ":" + connectInfo.password;
		curl_easy_setopt(m_curlContext, CURLOPT_USERPWD, usrPwdString.c_str());
		curl_easy_setopt(m_curlContext, CURLOPT_UPLOAD, 1L);
	}

	void CommonPlatformFTPClient::UploadStringAsFile(const Filesystem::Path& targetFilepath, const String& dataStr)
	{
		const String finalURL = FormatString("ftp://{}/{}", m_connectionInfo.url, targetFilepath.ToString());
		UploadData uploadData = { reinterpret_cast<const uint8_t*>(dataStr.c_str()), dataStr.size(), 0 };

		curl_easy_setopt(m_curlContext, CURLOPT_URL, finalURL.c_str());
		curl_easy_setopt(m_curlContext, CURLOPT_READFUNCTION, DataReadCallback);
		curl_easy_setopt(m_curlContext, CURLOPT_READDATA, &uploadData);
		curl_easy_setopt(m_curlContext, CURLOPT_USE_SSL, CURLUSESSL_ALL);
		curl_easy_setopt(m_curlContext, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(m_curlContext, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(m_curlContext, CURLOPT_PORT, 21);
		curl_easy_setopt(m_curlContext, CURLOPT_SSL_SESSIONID_CACHE, 1L);
		curl_easy_setopt(m_curlContext, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(uploadData.size));
	
		CURLcode result = curl_easy_perform(m_curlContext);
		if (result != CURLE_OK)
		{
			VT_LOG(Warning, "{}", curl_easy_strerror(result));
		}
	}

	// #TODO_Ivar: Probably not completely thread safe...
	void CommonPlatformFTPClient::Initialize()
	{
		if (!s_curlInitiailized)
		{
			curl_global_init(CURL_GLOBAL_DEFAULT);
			s_curlInitiailized = true;
			s_numActiveClients++;
		}
	}

	void CommonPlatformFTPClient::Cleanup()
	{
		if (m_curlContext)
		{
			curl_easy_cleanup(m_curlContext);
			m_curlContext = nullptr;
		}

		if (s_curlInitiailized && s_numActiveClients == 1)
		{
			s_numActiveClients = 0;
			s_curlInitiailized = false;

			curl_global_cleanup();
		}
	}
}
