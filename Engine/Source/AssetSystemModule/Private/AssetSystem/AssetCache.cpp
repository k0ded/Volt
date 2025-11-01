#include "aspch.h"

#include "AssetSystem/AssetCache.h"
#include "AssetSystem/AssetRegistry.h"

namespace Volt
{
	AssetCache::AssetCache()
	{
		Initialize();
	}

	AssetCache::~AssetCache()
	{
		VT_ENSURE_MSG(m_cache.empty(), "Cache should have been cleared before destruction!");
	}

	void AssetCache::Clear()
	{
		m_cache.clear();
	}

	void AssetCache::AddAsset(RefPtr<Asset> asset)
	{
		VT_ENSURE(asset->GetAssetHandle() != Asset::Null());

		uint64_t hashIndex;
		if (m_hashTable.Insert(asset->GetAssetHandle(), hashIndex))
		{
			m_cache[hashIndex] = asset;
		}
	}

	void AssetCache::RemoveAsset(AssetHandle assetHandle)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		uint64_t hashIndex;
		if (m_hashTable.GetAndRemove(assetHandle, hashIndex))
		{
			m_cache[hashIndex].Reset();
		}
		else
		{
			VT_LOGC(Warning, LogAssetSystem, "Trying to remove asset with handle '{}' from the asset cache, but it has not beed cached!", assetHandle);
		}
	}

	RefPtr<Asset> AssetCache::GetAsset(AssetHandle assetHandle)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		uint64_t hashIndex;
		if (m_hashTable.Get(assetHandle, hashIndex))
		{
			return m_cache[hashIndex];
		}

		return nullptr;
	}

	bool AssetCache::TryGetAsset(AssetHandle assetHandle, RefPtr<Asset>& outAsset)
	{
		uint64_t hashIndex;
		bool found = m_hashTable.Get(assetHandle, hashIndex);
		if (found)
		{
			outAsset = m_cache[hashIndex];
		}

		return found;
	}

	void AssetCache::Initialize()
	{
		m_hashTable.Reserve(AssetRegistry::GetNumMaxAssets());
		m_cache.resize(AssetRegistry::GetNumMaxAssets());
	}
}
