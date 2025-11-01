#pragma once

#include "Volt-Animation/Assets/Skeleton.h"

#include <AssetSystem/Serialization/AssetSerializer.h>
#include <AssetSystem/AssetSerializerRegistry.h>

namespace Volt
{
	class SkeletonSerializer : public AssetSerializer
	{
	public:
		void Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const override;
		bool Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset> destinationAsset) const override;
	};

	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::Skeleton, SkeletonSerializer);
}
