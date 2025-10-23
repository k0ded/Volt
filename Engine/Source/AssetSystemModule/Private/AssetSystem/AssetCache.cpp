#include "aspch.h"

#include "AssetSystem/AssetCache.h"

namespace Volt
{
	void AssetCache::AddAsset(RefPtr<Asset_New> asset)
	{
		VT_ENSURE(!m_assetMap.contains(asset->GetAssetHandle()));
		m_assetMap[asset->GetAssetHandle()] = asset;
	}

	void AssetCache::RemoveAsset(AssetHandle assetHandle)
	{
		VT_ENSURE(m_assetMap.contains(assetHandle));
		m_assetMap.erase(assetHandle);
	}

	RefPtr<Asset_New> AssetCache::GetAsset(AssetHandle assetHandle)
	{
		if (m_assetMap.contains(assetHandle))
		{
			return m_assetMap.at(assetHandle);
		}

		return nullptr;
	}

	bool AssetCache::TryGetAsset(AssetHandle assetHandle, RefPtr<Asset_New>& outAsset)
	{
		if (m_assetMap.contains(assetHandle))
		{
			outAsset = m_assetMap.at(assetHandle);
			return true;
		}

		return false;
	}
}
