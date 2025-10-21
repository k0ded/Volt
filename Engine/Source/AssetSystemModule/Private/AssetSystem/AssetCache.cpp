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
}
