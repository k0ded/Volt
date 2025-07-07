#pragma once

#include "Volt-Animation/Assets/Skeleton.h"

#include <AssetSystem/Serialization/AssetSerializer.h>
#include <AssetSystem/AssetSerializerRegistry.h>

namespace Volt
{
	class SkeletonSerializer : public AssetSerializer
	{
	public:
		void Serialize(const AssetMetadata& metadata, StackVector<uint8_t, ASSET_METADATA_SIZE>& customData, const Ref<Asset>& asset) const override;
		bool Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const override;
	};

	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::Skeleton, SkeletonSerializer);
}
