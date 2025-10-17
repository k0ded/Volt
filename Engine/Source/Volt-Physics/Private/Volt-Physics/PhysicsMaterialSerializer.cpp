#include "vppch.h"
#include "Volt-Physics/PhysicsMaterialSerializer.h"
#include "Volt-Physics/PhysicsMaterialAsset.h"

#include <AssetSystem/AssetManager.h>

#include <PhysicsInterface/PhysicsMaterial.h>

namespace Volt
{
	struct PhysicsMaterialSerializationData
	{
		float staticFriction;
		float dynamicFriction;
		float bounciness;
	};

	void PhysicsMaterialSerializer::Serialize(const AssetMetadata& metadata, CustomAssetMetadataVector& customData, const Ref<Asset>& asset) const
	{
		Ref<PhysicsMaterialAsset> material = std::reinterpret_pointer_cast<PhysicsMaterialAsset>(asset);

		BinaryStreamWriter streamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(metadata, asset->GetVersion(), streamWriter);

		PhysicsMaterialSerializationData serializationData{};
		serializationData.staticFriction = material->m_material->GetStaticFriction();
		serializationData.dynamicFriction = material->m_material->GetDynamicFriction();
		serializationData.bounciness = material->m_material->GetBounciness();

		streamWriter.Write(serializationData);

		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool PhysicsMaterialSerializer::Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const
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

		PhysicsMaterialSerializationData serializationData{};
		streamReader.Read(serializationData);

		Ref<PhysicsMaterial> physicsMat = std::reinterpret_pointer_cast<PhysicsMaterial>(destinationAsset);
		physicsMat->SetStaticFriction(serializationData.staticFriction);
		physicsMat->SetDynamicFriction(serializationData.dynamicFriction);
		physicsMat->SetBounciness(serializationData.bounciness);

		return true;
	}
}
