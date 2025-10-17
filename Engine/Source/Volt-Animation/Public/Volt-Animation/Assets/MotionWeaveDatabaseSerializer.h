#pragma once

#include "Volt-Animation/Assets/MotionWeaveDatabase.h"

#include <AssetSystem/Serialization/AssetSerializer.h>
#include <AssetSystem/AssetSerializerRegistry.h>

namespace Volt
{
	class MotionWeaveDatabaseSerializer : public AssetSerializer
	{
	public:
		void Serialize(const AssetMetadata& metadata, CustomAssetMetadataVector& customData, const Ref<Asset>& asset) const override;
		bool Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const override;
	};

	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::MotionWeave, MotionWeaveDatabaseSerializer);
}
