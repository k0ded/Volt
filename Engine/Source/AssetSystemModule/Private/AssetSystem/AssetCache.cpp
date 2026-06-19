#include "aspch.h"

#include "AssetSystem/AssetCache.h"
#include "AssetSystem/AssetRegistry.h"

#include <CoreUtilities/ConsoleVariableRegistry.h>

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
		VT_ENSURE_MSG(m_hashTable.IsEmpty(), "Cache should have been cleared before destruction!");
	}

	void AssetCache::Clear()
	{
		m_hashTable.Clear();
	}

	bool AssetCache::TryPublish(AssetHandle assetHandle, IntRef<Asset> asset, uint64_t generation)
	{
		const size_t hash = GetAssetHash(assetHandle, generation);

		auto InsertIntoContainer = [&](Container* container) 
		{
			container->asset.store(asset.GetRaw(), std::memory_order::relaxed);

			if (s_assetCacheLog.GetValue())
			{
				VT_LOGC(Trace, LogAssetSystem,
					"Added asset '{}' (Handle: '{}', Type: '{}', Generation: '{}') to cache",
					asset->GetAssetName(),
					asset->GetAssetHandle(),
					asset->GetType()->GetName(),
					generation);
			}
		};

		Optional<Container*> value = m_hashTable.Find(hash);
		if (value.HasValue())
		{
			InsertIntoContainer(value.Get());
			return true;
		}

		Container* newContainer = m_allocator.Allocate();
		value = m_hashTable.GetOrInsert(hash, newContainer);

		if (value.HasValue())
		{
			if (value.Get() != newContainer)
			{
				m_allocator.Free(newContainer);
			}

			InsertIntoContainer(value.Get());
			return true;
		}

		return false;
	}

	bool AssetCache::TryRemove(AssetHandle assetHandle, uint64_t generation)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		const size_t hash = GetAssetHash(assetHandle, generation);

		Optional<Container*> container = m_hashTable.GetAndErase(hash);

		if (container.HasValue())
		{
			Asset* asset = container.Get()->asset.exchange(nullptr, std::memory_order::relaxed);

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

			m_allocator.Free(container.Get());
		}
		else
		{
			VT_LOGC(Warning, LogAssetSystem, "Trying to remove asset with handle '{}' from the asset cache, but it has not been cached!", assetHandle);
		}

		return container.HasValue();
	}

	bool AssetCache::TryGet(AssetHandle assetHandle, uint64_t generation, IntRef<Asset>& outAsset)
	{
		const size_t hash = GetAssetHash(assetHandle, generation);

		Optional<Container*> container = m_hashTable.Find(hash);
		if (container.HasValue())
		{
			Asset* assetPtr = container.Get()->asset.load(std::memory_order::relaxed);

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
				outAsset = IntRef<Asset>::Attach(assetPtr);
			}
			else
			{
				return false;
			}
		}

		return container.HasValue();
	}

	void AssetCache::Initialize()
	{
		m_hashTable.Reserve(AssetRegistry::GetNumMaxAssets());
	}
}
