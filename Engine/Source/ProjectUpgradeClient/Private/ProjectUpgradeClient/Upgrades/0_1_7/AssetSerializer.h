#pragma once

#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializationCommon.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetMetadata_0_1_7.h"
#include "ProjectUpgradeClient/Upgrades/Common/BinaryStreamWriter.h"
#include "ProjectUpgradeClient/Upgrades/Common/BinaryStreamReader.h"

#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetReference.h>
#include <AssetSystem/AssetRegistry.h>

namespace Volt
{
	class AssetSerializer
	{
	public:
		virtual ~AssetSerializer() = default;

		virtual void Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& outCustomData, const AssetReference<Asset>& asset) const = 0;
		virtual bool Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const = 0;

		[[nodiscard]] static size_t WriteMetadata(const AssetMetadata_0_1_7& metadata, const uint32_t version, BinaryStreamWriter& streamWriter);
		static SerializedAssetMetadata ReadMetadata(BinaryStreamReader& streamReader);
	};
}
