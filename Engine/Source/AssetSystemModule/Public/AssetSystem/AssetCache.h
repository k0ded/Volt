#pragma once

#include "AssetSystem/Asset.h"

#include <CoreUtilities/Containers/AtomicHashTable.h>
#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
	class AssetCache
	{
	public:
		VTAS_API AssetCache();
		VTAS_API ~AssetCache();

		VTAS_API void Clear();

		VTAS_API bool TryPublish(AssetHandle assetHandle, RefPtr<Asset> asset, uint64_t generation);
		VTAS_API bool TryRemove(AssetHandle assetHandle, uint64_t generation);
		VTAS_API bool TryGet(AssetHandle assetHandle, uint64_t generation, RefPtr<Asset>& outAsset);

	private:
		struct Container
		{
			Container() = default;
			Container(const Container& other)
				: asset(other.asset.load(std::memory_order::relaxed))
			{}

			Container& operator=(const Container& other)
			{
				if (this != &other)
				{
					asset = other.asset.load(std::memory_order::relaxed);
				}

				return *this;
			}

			std::atomic<Asset*> asset = nullptr;
		};

		void Initialize();

		AtomicHashTable<> m_hashTable;
		Vector<Container> m_cache;
	};
}
