#include "vapch.h"

#include "Volt-Animation/Assets/SkeletonSerializer.h"
#include "Volt-Animation/Assets/Skeleton.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

namespace Volt
{
	struct SkeletonSerializationData
	{
		std::string name;
		Vector<Skeleton::Joint> joints;
		Map<std::string, size_t> jointNameToIndex;
		Vector<Skeleton::JointAttachment> jointAttachments;
		Vector<glm::mat4> inverseBindPose;
		Vector<Animation::TRS> restPose;

		static void Serialize(BinaryStreamWriter& streamWriter, const SkeletonSerializationData& data)
		{
			streamWriter.Write(data.name);
			streamWriter.Write(data.joints);
			streamWriter.Write(data.jointNameToIndex);
			streamWriter.Write(data.jointAttachments);
			streamWriter.Write(data.inverseBindPose);
			streamWriter.WriteRaw(data.restPose);
		}

		static void Deserialize(BinaryStreamReader& streamReader, SkeletonSerializationData& outData)
		{
			streamReader.Read(outData.name);
			streamReader.Read(outData.joints);
			streamReader.Read(outData.jointNameToIndex);
			streamReader.Read(outData.jointAttachments);
			streamReader.Read(outData.inverseBindPose);
			streamReader.ReadRaw(outData.restPose);
		}
	};

	void SkeletonSerializer::Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		AssetReference<Skeleton> skeleton = asset.ConvertTo<Skeleton>();
		ScopedAssetReferenceLock skeletonLock{ skeleton };

		BinaryStreamWriter streamWriter{};
	
		SkeletonSerializationData serializationData{};
		serializationData.name = skeleton->m_name;
		serializationData.joints = skeleton->m_joints;
		serializationData.jointAttachments = skeleton->m_jointAttachments;
		serializationData.inverseBindPose = skeleton->m_inverseBindPose;
		serializationData.restPose = skeleton->m_restPose;
		serializationData.jointNameToIndex = skeleton->m_jointNameToIndex;

		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(*metadata, asset->GetVersion(), streamWriter);
		streamWriter.Write(serializationData);

		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool SkeletonSerializer::Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset> destinationAsset) const
	{
		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);

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

		SkeletonSerializationData serializationData{};
		streamReader.Read(serializationData);

		AssetReference<Skeleton> skeleton = destinationAsset.ConvertTo<Skeleton>();
		ScopedAssetReferenceLock skeletonLock{ skeleton };

		skeleton->m_name = serializationData.name;
		skeleton->m_joints = serializationData.joints;
		skeleton->m_jointAttachments = serializationData.jointAttachments;
		skeleton->m_inverseBindPose = serializationData.inverseBindPose;
		skeleton->m_restPose = serializationData.restPose;
		skeleton->m_jointNameToIndex = serializationData.jointNameToIndex;

		return true;
	}
}
