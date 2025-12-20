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
		void Initialize();

		AtomicHashTable<> m_hashTable;
		Vector<Asset*> m_cache;
	};
}
