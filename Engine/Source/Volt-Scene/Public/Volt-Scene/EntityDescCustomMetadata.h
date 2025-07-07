#pragma once
#include "Volt-Scene/AssetTypes.h"

#include <AssetSystem/AssetType.h>
#include <AssetSystem/AssetHandle.h>

namespace Volt
{
	struct EntityDescCustomMetadata
	{
		static bool IsForAssetType(const AssetType& assetType) { return assetType->GetGUID() == AssetTypes::EntityDesc->GetGUID(); }

		Volt::AssetHandle sceneHandle;
	};
}
