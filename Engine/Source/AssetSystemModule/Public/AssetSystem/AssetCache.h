#pragma once

#include "AssetSystem/Asset.h"

#include <CoreUtilities/Containers/AtomicHashTable.h>
#include <CoreUtilities/Pointers/IntRef.h>

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

namespace Volt
{
	class AssetCache
	{
	public:
		struct Container
		{
			Container() = default;
			Container(const Container& other)
				: asset(other.asset)
			{}

			Container& operator=(const Container& other)
			{
				if (this != &other)
				{
					asset = other.asset;
				}

				return *this;
			}

			Asset* asset = nullptr;
		};

		VTAS_API AssetCache();
		VTAS_API ~AssetCache();

		VTAS_API void Clear();

		VTAS_API bool TryPublish(AssetHandle assetHandle, IntRef<Asset> asset, uint64_t generation);
		VTAS_API bool TryGet(AssetHandle assetHandle, uint64_t generation, IntRef<Asset>& outAsset);

		VTAS_API Container* Evict(AssetHandle assetHandle, uint64_t generation);
		VTAS_API void FreeContainer(Container* container);

	private:
		void Initialize();

		AtomicHashTable<Container*> m_hashTable;
		PagedAtomicArenaAllocator<Container, 1024> m_allocator;
	};
}
