#include "vrpch.h"

#include "Volt-Renderer/AnimatedCharacterSerializer.h"
#include "Volt-Renderer/AnimatedCharacter.h"
#include <Volt-Animation/Assets/Skeleton.h>

#include <AssetSystem/AssetManager.h>

namespace Volt
{
	struct SerializedAnimationEvent
	{
		uint32_t frameIndex;
		std::string name;

		static void Serialize(BinaryStreamWriter& streamWriter, const SerializedAnimationEvent& data)
		{
			streamWriter.Write(data.frameIndex);
			streamWriter.Write(data.name);
		}

		static void Deserialize(BinaryStreamReader& streamReader, SerializedAnimationEvent& outData)
		{
			streamReader.Read(outData.frameIndex);
			streamReader.Read(outData.name);
		}
	};

	struct SerializedJointAttachment
	{
		std::string name;
		int32_t jointIndex;
		UUID64 id;
		glm::vec3 positionOffset;
		glm::quat rotationOffset;

		static void Serialize(BinaryStreamWriter& streamWriter, const SerializedJointAttachment& data)
		{
			streamWriter.Write(data.name);
			streamWriter.Write(data.jointIndex);
			streamWriter.Write(data.id);
			streamWriter.Write(data.positionOffset);
			streamWriter.Write(data.rotationOffset);
		}

		static void Deserialize(BinaryStreamReader& streamReader, SerializedJointAttachment& outData)
		{
			streamReader.Read(outData.name);
			streamReader.Read(outData.jointIndex);
			streamReader.Read(outData.id);
			streamReader.Read(outData.positionOffset);
			streamReader.Read(outData.rotationOffset);
		}
	};

	struct SerializedAnimation
	{
		uint32_t index;
		AssetHandle animationHandle;

		Vector<SerializedAnimationEvent> events;

		static void Serialize(BinaryStreamWriter& streamWriter, const SerializedAnimation& data)
		{
			streamWriter.Write(data.index);
			streamWriter.Write(data.animationHandle);
			streamWriter.Write(data.events);
		}

		static void Deserialize(BinaryStreamReader& streamReader, SerializedAnimation& outData)
		{
			streamReader.Read(outData.index);
			streamReader.Read(outData.animationHandle);
			streamReader.Read(outData.events);
		}
	};

	struct AnimatedCharacterSerializationData
	{
		AssetHandle skeletonHandle;
		AssetHandle skinHandle;

		Vector<SerializedAnimation> animations;
		Vector<SerializedJointAttachment> jointAttachments;

		static void Serialize(BinaryStreamWriter& streamWriter, const AnimatedCharacterSerializationData& data)
		{
			streamWriter.Write(data.skeletonHandle);
			streamWriter.Write(data.skinHandle);
			streamWriter.Write(data.animations);
			streamWriter.Write(data.jointAttachments);
		}

		static void Deserialize(BinaryStreamReader& streamReader, AnimatedCharacterSerializationData& outData)
		{
			streamReader.Read(outData.skeletonHandle);
			streamReader.Read(outData.skinHandle);
			streamReader.Read(outData.animations);
			streamReader.Read(outData.jointAttachments);
		}
	};

	void AnimatedCharacterSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<AnimatedCharacter> character = std::reinterpret_pointer_cast<AnimatedCharacter>(asset);

		AnimatedCharacterSerializationData serializationData{};
		serializationData.skeletonHandle = character->m_skeleton ? character->m_skeleton->handle : Asset::Null();
		serializationData.skinHandle = character->m_skin ? std::reinterpret_pointer_cast<Asset>(character->m_skin)->handle : Asset::Null();

		serializationData.animations.reserve(character->m_animations.size());
		for (const auto& [index, anim] : character->m_animations)
		{
			auto& serAnim = serializationData.animations.emplace_back();
			serAnim.index = index;
			serAnim.animationHandle = anim ? anim->handle : Asset::Null();

			if (character->m_animationEvents.contains(index))
			{
				serAnim.events.reserve(character->m_animationEvents.at(index).size());
				for (const auto& event : character->m_animationEvents.at(index))
				{
					auto& serEvent = serAnim.events.emplace_back();
					serEvent.name = event.name;
					serEvent.frameIndex = event.frame;
				}
			}
		}

		serializationData.jointAttachments.reserve(character->m_jointAttachments.size());
		for (const auto& jntAttachment : character->m_jointAttachments)
		{
			auto& serAttachment = serializationData.jointAttachments.emplace_back();
			serAttachment.name = jntAttachment.name;
			serAttachment.jointIndex = jntAttachment.jointIndex;
			serAttachment.id = jntAttachment.id;
			serAttachment.positionOffset = jntAttachment.positionOffset;
			serAttachment.rotationOffset = jntAttachment.rotationOffset;
		}

		BinaryStreamWriter streamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(metadata, asset->GetVersion(), streamWriter);
		streamWriter.Write(serializationData);

		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool AnimatedCharacterSerializer::Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const
	{
		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);

		if (!std::filesystem::exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata.filePath);
			destinationAsset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader streamReader{ filePath };

		if (!streamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file: {0}!", metadata.filePath);
			destinationAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);

		VT_ASSERT_MSG(serializedMetadata.version == destinationAsset->GetVersion(), "Incompatible version!");

		AnimatedCharacterSerializationData serializationData{};
		streamReader.Read(serializationData);

		Ref<AnimatedCharacter> character = std::reinterpret_pointer_cast<AnimatedCharacter>(destinationAsset);
		character->m_skeleton = AssetManager::QueueAsset<Skeleton>(serializationData.skeletonHandle);
		//character->m_skin = AssetManager::QueueAsset<Mesh>(serializationData.skinHandle);

		AssetManager::AddDependencyToAsset(metadata.handle, serializationData.skeletonHandle);
		AssetManager::AddDependencyToAsset(metadata.handle, serializationData.skinHandle);

		for (const auto& serAnim : serializationData.animations)
		{
			for (const auto& serEvent : serAnim.events)
			{
				auto& newEvent = character->m_animationEvents[serAnim.index].emplace_back();
				newEvent.name = serEvent.name;
				newEvent.frame = serEvent.frameIndex;
			}

			if (serAnim.index != std::numeric_limits<uint32_t>::max() && serAnim.animationHandle != Asset::Null())
			{
				character->m_animations[serAnim.index] = AssetManager::QueueAsset<Animation>(serAnim.animationHandle);
				AssetManager::AddDependencyToAsset(metadata.handle, serAnim.animationHandle);
			}
		}

		for (const auto& serJntAttachment : serializationData.jointAttachments)
		{
			auto& newAttachment = character->m_jointAttachments.emplace_back();
			newAttachment.name = serJntAttachment.name;
			newAttachment.jointIndex = serJntAttachment.jointIndex;
			newAttachment.id = serJntAttachment.id;
			newAttachment.positionOffset = serJntAttachment.positionOffset;
			newAttachment.rotationOffset = serJntAttachment.rotationOffset;
		}

		return true;
	}
}
