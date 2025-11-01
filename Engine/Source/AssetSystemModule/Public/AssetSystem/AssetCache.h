#pragma once

#include "AssetSystem/Asset_New.h"

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

		VTAS_API void AddAsset(RefPtr<Asset> asset);
		VTAS_API void RemoveAsset(AssetHandle assetHandle);

		VTAS_API RefPtr<Asset> GetAsset(AssetHandle assetHandle);
		VTAS_API bool TryGetAsset(AssetHandle assetHandle, RefPtr<Asset>& outAsset);

	private:
		void Initialize();

		AtomicHashTable<> m_hashTable;
		Vector<RefPtr<Asset>> m_cache;
	};
}
