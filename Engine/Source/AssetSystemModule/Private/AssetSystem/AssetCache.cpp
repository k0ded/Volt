#include "aspch.h"

#include "AssetSystem/AssetCache.h"
#include "AssetSystem/AssetRegistry.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

namespace Volt
{
	static ConsoleVariable<int32_t> s_assetCacheLog(
		"a.AssetCache.Log",
		0,
		"Whether or not to log asset cache interactions.");

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

			if (s_assetCacheLog.GetValue())
			{
				VT_LOGC(Trace, LogAssetSystem,
					"Added asset '{}' (Handle: '{}', Type: '{}') to cache",
					asset->GetAssetName(),
					asset->GetAssetHandle(),
					asset->GetType()->GetName());
			}
		}
		else
		{
			VT_LOGC(Error, LogAssetSystem, "Unable to cache asset with handle '{}'", asset->GetAssetHandle());
		}
	}

	void AssetCache::RemoveAsset(AssetHandle assetHandle)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		uint64_t hashIndex;
		if (m_hashTable.GetAndRemove(assetHandle, hashIndex))
		{
			if (s_assetCacheLog.GetValue())
			{
				VT_LOGC(Trace, LogAssetSystem,
					"Removed asset '{}' (Handle: '{}', Type: '{}') from cache",
					m_cache[hashIndex]->GetAssetName(),
					m_cache[hashIndex]->GetAssetHandle(),
					m_cache[hashIndex]->GetType()->GetName());
			}

			m_cache[hashIndex].Reset();
		}
		else
		{
			VT_LOGC(Warning, LogAssetSystem, "Trying to remove asset with handle '{}' from the asset cache, but it has not been cached!", assetHandle);
		}
	}

	RefPtr<Asset> AssetCache::GetAsset(AssetHandle assetHandle)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		uint64_t hashIndex;
		if (m_hashTable.Get(assetHandle, hashIndex))
		{
			if (m_cache[hashIndex]->IsFlagSet(AssetFlag::Removed))
			{
				return nullptr;
			}

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
			if (m_cache[hashIndex]->IsFlagSet(AssetFlag::Removed))
			{
				return false;
			}

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
