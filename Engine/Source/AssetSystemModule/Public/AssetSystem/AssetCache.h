#pragma once

#include "AssetSystem/Asset_New.h"

#include <CoreUtilities/Containers/AtomicHashTable.h>
#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
	class AssetCache
	{
	public:
		VTAS_API void AddAsset(RefPtr<Asset_New> asset);
		void RemoveAsset(AssetHandle assetHandle);

	private:
		// Replace
		Map<AssetHandle, RefPtr<Asset_New>> m_assetMap;
	};
}
