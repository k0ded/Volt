#pragma once

#include <AssetSystem/AssetTypes.h>

#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializer.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializerRegistry.h"

namespace Volt
{
	class PrefabSerializer : public	AssetSerializer
	{
	public:
		PrefabSerializer();
		~PrefabSerializer() override;

		void Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const override;
		bool Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const override;
	private:
		inline static PrefabSerializer* s_instance = nullptr;
	};

	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::Prefab, PrefabSerializer);
}
