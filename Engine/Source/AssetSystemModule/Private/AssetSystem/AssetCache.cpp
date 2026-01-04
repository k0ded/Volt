#include "aspch.h"

#include "AssetSystem/AssetCache.h"
#include "AssetSystem/AssetRegistry.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <CoreUtilities/Math/Hash.h>

namespace Volt
{
	static ConsoleVariable<int32_t> s_assetCacheLog(
		"a.AssetCache.Log",
		0,
		"Whether or not to log asset cache interactions.");

	static size_t GetAssetHash(AssetHandle assetHandle, uint64_t generation)
	{
		return Math::HashCombine(assetHandle, std::hash<uint64_t>()(generation));
	}

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

	bool AssetCache::TryPublish(AssetHandle assetHandle, RefPtr<Asset> asset, uint64_t generation)
	{
		const size_t hash = GetAssetHash(assetHandle, generation);

		uint64_t hashIndex;
		const bool inserted = m_hashTable.Insert(hash, hashIndex);
		if (inserted)
		{
			m_cache[hashIndex].asset.store(asset.GetRaw(), std::memory_order::relaxed);

			if (s_assetCacheLog.GetValue())
			{
				VT_LOGC(Trace, LogAssetSystem,
					"Added asset '{}' (Handle: '{}', Type: '{}', Generation: '{}') to cache",
					asset->GetAssetName(),
					asset->GetAssetHandle(),
					asset->GetType()->GetName(),
					generation);
			}
		}
		else
		{
			VT_LOGC(Error, LogAssetSystem, "Unable to cache asset with handle '{}'", assetHandle);
		}

		return inserted;
	}

	bool AssetCache::TryRemove(AssetHandle assetHandle, uint64_t generation)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		const size_t hash = GetAssetHash(assetHandle, generation);

		uint64_t hashIndex;
		const bool found = m_hashTable.GetAndRemove(hash, hashIndex);
		if (found)
		{
			Asset* asset = m_cache[hashIndex].asset.exchange(nullptr, std::memory_order::relaxed);

			if (asset)
			{
				if (s_assetCacheLog.GetValue())
				{
					VT_LOGC(Trace, LogAssetSystem,
						"Removed asset '{}' (Handle: '{}', Type: '{}', Generation: '{}') from cache",
						asset->GetAssetName(),
						asset->GetAssetHandle(),
						asset->GetType()->GetName(),
						generation);
				}
			}
		}
		else
		{
			VT_LOGC(Warning, LogAssetSystem, "Trying to remove asset with handle '{}' from the asset cache, but it has not been cached!", assetHandle);
		}

		return found;
	}

	bool AssetCache::TryGet(AssetHandle assetHandle, uint64_t generation, RefPtr<Asset>& outAsset)
	{
		const size_t hash = GetAssetHash(assetHandle, generation);

		uint64_t hashIndex;
		bool found = m_hashTable.Get(hash, hashIndex);
		if (found)
		{
			Asset* assetPtr = m_cache[hashIndex].asset.load(std::memory_order::relaxed);

			// Asset hasn't been stored yet.
			if (assetPtr == nullptr)
			{
				return false;
			}

			// Make sure the asset has the correct generation (should be correct, since it is baked into the hash)
			if (assetPtr->m_generation < generation)
			{
				return false;
			}
	
			if (assetPtr->GetRefCount() > 0)
			{
				outAsset = RefPtr<Asset>::Attach(assetPtr);
			}
			else
			{
				return false;
			}
		}

		return found;
	}

	void AssetCache::Initialize()
	{
		m_hashTable.Reserve(AssetRegistry::GetNumMaxAssets());
		m_cache.resize(AssetRegistry::GetNumMaxAssets());
	}
}
