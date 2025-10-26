#pragma once

#include <AssetSystem/AssetTypes.h>

#include <AssetSystem/Serialization/AssetSerializer.h>
#include <AssetSystem/AssetSerializerRegistry.h>

namespace Volt
{
	class PrefabSerializer : public	AssetSerializer
	{
	public:
		PrefabSerializer();
		~PrefabSerializer() override;

		void Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset_New>& asset) const override;
		bool Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset_New> destinationAsset) const override;
	private:
		inline static PrefabSerializer* s_instance = nullptr;
	};

	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::Prefab, PrefabSerializer);
}
