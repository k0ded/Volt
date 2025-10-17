#pragma once

#include "Volt-Physics/PhysicsMaterialAsset.h"

#include <AssetSystem/Serialization/AssetSerializer.h>
#include <AssetSystem/AssetSerializerRegistry.h>

namespace Volt
{
	class PhysicsMaterialSerializer : public AssetSerializer
	{
	public:
		void Serialize(const AssetMetadata& metadata, CustomAssetMetadataVector& customData, const Ref<Asset>& asset) const override;
		bool Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const override;
	};

	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::PhysicsMaterial, PhysicsMaterialSerializer);
}
