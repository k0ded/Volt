#pragma once

#include "AssetSystem/Asset_New.h"
#include "AssetSystem/AssetReference.h"
#include "AssetSystem/Serialization/AssetSerializationCommon.h"
#include "AssetSystem/AssetRegistry.h"

#include <CoreUtilities/FileIO/BinaryStreamWriter.h>
#include <CoreUtilities/FileIO/BinaryStreamReader.h>

namespace Volt
{
	class VTAS_API AssetSerializer
	{
	public:
		virtual ~AssetSerializer() = default;

		virtual void Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& outCustomData, const AssetReference<Asset>& asset) const = 0;
		virtual bool Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset> destinationAsset) const = 0;

		[[nodiscard]] static size_t WriteMetadata(const AssetMetadata& metadata, const uint32_t version, BinaryStreamWriter& streamWriter);
		static SerializedAssetMetadata ReadMetadata(BinaryStreamReader& streamReader);
	};
}
