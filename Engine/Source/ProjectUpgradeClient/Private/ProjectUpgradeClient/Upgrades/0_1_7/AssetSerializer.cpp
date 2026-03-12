#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializer.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializationCommon.h"

namespace Volt
{
	size_t AssetSerializer::WriteMetadata(const AssetMetadata_0_1_7& metadata, const uint32_t version, BinaryStreamWriter& streamWriter)
	{
		SerializedAssetMetadata serializedMetadata{};
		serializedMetadata.handle = metadata.handle;
		serializedMetadata.type = metadata.type;
		serializedMetadata.version = version;
		serializedMetadata.customData = metadata.customData;

		//call reserve here to set the begin ptr
		serializedMetadata.customData.reserve(ASSET_CUSTOM_METADATA_SIZE);

		streamWriter.Write(SerializedAssetMetadata::AssetMagic);
		return streamWriter.Write(serializedMetadata);
	}

	SerializedAssetMetadata AssetSerializer::ReadMetadata(BinaryStreamReader& streamReader)
	{
		uint32_t magic;
		streamReader.Read(magic);
		VT_ASSERT(magic == SerializedAssetMetadata::AssetMagic);

		SerializedAssetMetadata result{};
		streamReader.Read(result);
		return result;
	}
}
