#pragma once

#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializer.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializerRegistry.h"

#include <Volt-Animation/Assets/AssetTypes.h>

namespace Volt
{
	class AnimationSerializer : public AssetSerializer
	{
	public:
		void Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const override;
		bool Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset> destinationAsset) const override;
	};

	inline static bool AssetSerializer_AnimationSerializer_Registered = AssetSerializerRegistry::Get().RegisterAssetSerializer(AssetTypes::AnimationType::guid, CreateRef<AnimationSerializer>());
}
