#include "vapch.h"

#include "Volt-Animation/Assets/BlendSpaceSerializer.h"
#include "Volt-Animation/BlendSpace.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

namespace Volt
{
	struct SerializedAnimation
	{
		AssetHandle handle;
		glm::vec2 value;
	};

	struct BlendSpaceSerializationData
	{
		BlendSpaceDimension dimension;
		glm::vec2 horizontalValues;
		glm::vec2 verticalValues;

		Vector<SerializedAnimation> animations;

		static void Serialize(BinaryStreamWriter& streamWriter, const BlendSpaceSerializationData& data)
		{
			streamWriter.Write(data.dimension);
			streamWriter.Write(data.horizontalValues);
			streamWriter.Write(data.verticalValues);
			streamWriter.WriteRaw(data.animations);
		}

		static void Deserialize(BinaryStreamReader& streamReader, BlendSpaceSerializationData& outData)
		{
			streamReader.Read(outData.dimension);
			streamReader.Read(outData.horizontalValues);
			streamReader.Read(outData.verticalValues);
			streamReader.ReadRaw(outData.animations);
		}
	};

	void BlendSpaceSerializer::Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		AssetReference<BlendSpace> blendSpace = asset.ConvertTo<BlendSpace>();
		ScopedAssetReferenceLock blendSpaceLock{ blendSpace };

		BlendSpaceSerializationData serializationData{};
		serializationData.dimension = blendSpace->m_dimension;
		serializationData.horizontalValues = blendSpace->m_horizontalValues;
		serializationData.verticalValues = blendSpace->m_verticalValues;

		for (const auto& anim : blendSpace->myAnimations)
		{
			auto& serAnim = serializationData.animations.emplace_back();
			serAnim.handle = anim.second;
			serAnim.value = anim.first;
		}

		BinaryStreamWriter streamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(*metadata, asset->GetVersion(), streamWriter);

		streamWriter.Write(serializationData);

		const auto filePath = g_assetManager->GetFilesystemPath(metadata->filepath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool BlendSpaceSerializer::Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset> destinationAsset) const
	{
		const auto filePath = g_assetManager->GetFilesystemPath(metadata->filepath);

		if (!std::filesystem::exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata->filepath);
			destinationAsset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader streamReader{ filePath };

		if (!streamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file: {0}!", metadata->filepath);
			destinationAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);
		VT_ASSERT_MSG(serializedMetadata.version == destinationAsset->GetVersion(), "Incompatible version!");

		BlendSpaceSerializationData serializationData{};
		streamReader.Read(serializationData);

		AssetReference<BlendSpace> blendSpace = destinationAsset.ConvertTo<BlendSpace>();
		ScopedAssetReferenceLock blendSpaceLock{ blendSpace };

		for (const auto& serAnim : serializationData.animations)
		{
			blendSpace->myAnimations.emplace_back(serAnim.value, serAnim.handle);
		}

		blendSpace->m_dimension = serializationData.dimension;
		blendSpace->m_horizontalValues = serializationData.horizontalValues;
		blendSpace->m_verticalValues = serializationData.verticalValues;

		return true;
	}
}
