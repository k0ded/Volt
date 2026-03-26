#pragma once

#ifdef _WIN32
#include <wrl.h>
#else
#include <dxc/WinAdapter.h>
#endif

#include <dxc/dxcapi.h>

#include <unordered_set>
#include <filesystem>

namespace Volt::RHI
{
	class HLSLIncluder : public IDxcIncludeHandler
	{
	public:
		HLSLIncluder();
		~HLSLIncluder();

		HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override;
		HRESULT QueryInterface(const IID& riid, void** ppvObject) override { return m_defaultIncludeHandler->QueryInterface(riid, ppvObject); }

		ULONG AddRef() override { return 0; }
		ULONG Release() override { return 0; }

		const std::unordered_set<Filesystem::Path>& GetIncludedFiles() const { return m_includedFiles; }

	private:
		IDxcIncludeHandler* m_defaultIncludeHandler = nullptr;
		IDxcUtils* m_hlslUtils = nullptr;

		std::unordered_set<Filesystem::Path> m_includedFiles;
	};
}
