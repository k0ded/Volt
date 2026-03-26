#include "ProjectUpgradeClient/Upgrades/0_1_7/Serializers/AnimationSerializer.h"

#define private public
#include <Volt-Animation/Assets/Animation.h>
#undef private

#include <Volt-FileSystem/Filesystem.h>

#include <AssetSystem/AssetManager.h>

namespace Volt
{
	struct AnimationSerializationData
	{
		float duration;
		uint32_t framesPerSecond;
		Vector<Animation::Pose> frames;
		Vector<Animation::Event> events;
	};

	static void Serialize(BinaryStreamWriter& streamWriter, const AnimationSerializationData& data)
	{
		streamWriter.Write(data.duration);
		streamWriter.Write(data.framesPerSecond);
		streamWriter.Write(data.frames);
		streamWriter.Write(data.events);
	}

	static void Deserialize(BinaryStreamReader& streamReader, AnimationSerializationData& outData)
	{
		streamReader.Read(outData.duration);
		streamReader.Read(outData.framesPerSecond);
		streamReader.Read(outData.frames);
		streamReader.Read(outData.events);
	}

	static void Serialize(BinaryStreamWriter& streamWriter, const Animation::TRS& data)
	{
		streamWriter.Write(data.translation);
		streamWriter.Write(data.rotation);
		streamWriter.Write(data.scale);
	}

	static void Deserialize(BinaryStreamReader& streamReader, Animation::TRS& outData)
	{
		streamReader.Read(outData.translation);
		streamReader.Read(outData.rotation);
		streamReader.Read(outData.scale);
	}

	static void Serialize(BinaryStreamWriter& streamWriter, const Animation::Pose& data)
	{
		streamWriter.WriteRaw(data.localTRS);
	}

	static void Deserialize(BinaryStreamReader& streamReader, Animation::Pose& outData)
	{
		streamReader.ReadRaw(outData.localTRS);
	}

	static void Serialize(BinaryStreamWriter& streamWriter, const Animation::Event& data)
	{
		streamWriter.Write(data.frame);
		streamWriter.Write(data.name);
	}

	static void Deserialize(BinaryStreamReader& streamReader, Animation::Event& outData)
	{
		streamReader.Read(outData.frame);
		streamReader.Read(outData.name);
	}

	void AnimationSerializer::Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		AssetReference<Animation> animation = asset.ConvertTo<Animation>();

		BinaryStreamWriter streamWriter{};

		AnimationSerializationData serializationData{};
		serializationData.duration = animation->m_duration;
		serializationData.framesPerSecond = animation->m_framesPerSecond;
		serializationData.frames = animation->m_frames;
		serializationData.events = animation->m_events;
	
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(*metadata, asset->GetVersion(), streamWriter);
		streamWriter.Write(serializationData);

		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool AnimationSerializer::Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const
	{
		const auto filepath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);

		if (!Filesystem::Exists(filepath))
		{
			VT_LOG(Error, "File {0} not found!", metadata->filepath);
			destinationAsset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader streamReader{ filepath };

		if (!streamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file: {0}!", metadata->filepath);
			destinationAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);
		VT_ASSERT_MSG(serializedMetadata.version == destinationAsset->GetVersion(), "Incompatible version!");

		AnimationSerializationData serializationData{};
		streamReader.Read(serializationData);

		AssetReference<Animation> animation = destinationAsset.ConvertTo<Animation>();

		animation->m_duration = serializationData.duration;
		animation->m_framesPerSecond = serializationData.framesPerSecond;
		animation->m_frames = serializationData.frames;
		animation->m_events = serializationData.events;

		return true;
	}
}
