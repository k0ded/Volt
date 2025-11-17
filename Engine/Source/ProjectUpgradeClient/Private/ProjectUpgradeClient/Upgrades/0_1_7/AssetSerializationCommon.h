#pragma once

#include <AssetSystem/Config.h>
#include <AssetSystem/AssetMetadata.h>

class BinaryStreamWriter;
class BinaryStreamReader;

namespace Volt
{
	struct SerializedAssetMetadata
	{
		inline static constexpr uint32_t AssetMagic = 9999;
		inline static constexpr size_t HeaderSize = sizeof(AssetType) + sizeof(uint32_t) + sizeof(AssetHandle) + sizeof(TypeHeader) * 9 + ASSET_CUSTOM_METADATA_SIZE + sizeof(TypeHeader) * 2;

		AssetType type;
		uint32_t version;
		AssetHandle handle;

		CustomAssetMetadataVector customData; // asset specific Metadata

		static void Serialize(BinaryStreamWriter& streamWriter, const SerializedAssetMetadata& data);
		static void Deserialize(BinaryStreamReader& streamReader, SerializedAssetMetadata& outData);
	};
}
