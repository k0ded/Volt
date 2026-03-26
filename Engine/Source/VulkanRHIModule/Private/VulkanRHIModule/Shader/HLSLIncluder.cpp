#include "vkpch.h"
#include "VulkanRHIModule/Shader/HLSLIncluder.h"

#include <Volt-FileSystem/Filesystem.h>
#include <Volt-FileSystem/FileUtility.h>

namespace Volt::RHI
{
	HLSLIncluder::HLSLIncluder()
	{
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_hlslUtils));
		HRESULT result = m_hlslUtils->CreateDefaultIncludeHandler(&m_defaultIncludeHandler);
		
		if (result != S_OK)
		{
			VT_ASSERT_MSG(false, "Failed to create default HLSL include handler");
		}
	}

	HLSLIncluder::~HLSLIncluder()
	{
		m_defaultIncludeHandler->Release();
		m_hlslUtils->Release();
	}

	HRESULT HLSLIncluder::LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource)
	{
		const Filesystem::Path filepath = pFilename;
		if (m_includedFiles.contains(filepath))
		{
			static const char nullStr[] = " ";

			IDxcBlobEncoding* encoding = nullptr;
			m_hlslUtils->CreateBlob(nullStr, ARRAYSIZE(nullStr), CP_UTF8, &encoding);

			*ppIncludeSource = encoding;

			return S_OK;
		}

		if (!Filesystem::Exists(filepath))
		{
			// #TODO_Ivar: Should error if not found in any of the directories.
			return S_FALSE;
		}

		String data;
		if (!FileUtility::ReadStringFromFile(filepath, data))
		{
			VT_LOGC(Error, LogVulkanRHI, FormatString("Failed to read file {0}!", filepath.ToString()));
			return S_FALSE;
		}

		m_includedFiles.insert(filepath);

		IDxcBlobEncoding* encoding = nullptr;
		m_hlslUtils->CreateBlob(data.data(), static_cast<uint32_t>(data.size()), CP_UTF8, &encoding);

		*ppIncludeSource = encoding;
		return S_OK;
	}
}
