#pragma once

#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializerRegistry.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializer.h"

#include <AssetSystem/AssetTypes.h>

namespace Volt
{
	class PhysicsMaterialSerializer : public AssetSerializer
	{
	public:
		void Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const override;
		bool Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset> destinationAsset) const override;
	};

	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::PhysicsMaterial, PhysicsMaterialSerializer);
}
