#pragma once
#include "Volt-Scene/AssetTypes.h"

#include <EntitySystem/EntityID.h>

#include <AssetSystem/AssetType.h>
#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/CustomAssetMetadataRegistry.h>


namespace Volt
{
	struct EntityDescCustomMetadata
	{
		static bool IsForAssetType(const AssetType& assetType) { return assetType->GetGUID() == AssetTypes::EntityDesc->GetGUID(); }

		Volt::AssetHandle sceneHandle;
		EntityID entityID;

		VT_INLINE friend Archive& operator<<(Archive& archive, EntityDescCustomMetadata& value)
		{
			archive << value.sceneHandle;
			archive << value.entityID;

			return archive;
		}
	};
}
