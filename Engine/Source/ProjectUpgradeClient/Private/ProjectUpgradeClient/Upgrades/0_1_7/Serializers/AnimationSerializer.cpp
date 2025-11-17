#include "ProjectUpgradeClient/Upgrades/0_1_7/Serializers/AnimationSerializer.h"

#define private public
#include <Volt-Animation/Assets/Animation.h>
#undef private

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

namespace Volt
{
	struct AnimationSerializationData
	{
		float duration;
		uint32_t framesPerSecond;
		Vector<Animation::Pose> frames;
		Vector<Animation::Event> events;

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
	};

	void AnimationSerializer::Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		AssetReference<Animation> animation = asset.ConvertTo<Animation>();
		ScopedAssetReferenceLock animationLock{ animation };

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

	bool AnimationSerializer::Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset> destinationAsset) const
	{
		const auto filepath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);

		if (!std::filesystem::exists(filepath))
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
		destinationAsset.Lock();
		VT_ASSERT_MSG(serializedMetadata.version == destinationAsset->GetVersion(), "Incompatible version!");
		destinationAsset.Unlock();

		AnimationSerializationData serializationData{};
		streamReader.Read(serializationData);

		AssetReference<Animation> animation = destinationAsset.ConvertTo<Animation>();
		ScopedAssetReferenceLock animationLock{ animation };

		animation->m_duration = serializationData.duration;
		animation->m_framesPerSecond = serializationData.framesPerSecond;
		animation->m_frames = serializationData.frames;
		animation->m_events = serializationData.events;

		return true;
	}
}
