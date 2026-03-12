#pragma once

#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializer.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializerRegistry.h"

#include <Volt-Animation/Assets/AssetTypes.h>

namespace Volt
{
	class SkeletonSerializer : public AssetSerializer
	{
	public:
		void Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const override;
		bool Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const override;
	};

	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::Skeleton, SkeletonSerializer);
}
