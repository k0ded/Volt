#include "vppch.h"
#include "Volt-Physics/PhysicsMaterialSerializer.h"
#include "Volt-Physics/PhysicsMaterialAsset.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

#include <PhysicsInterface/PhysicsMaterial.h>

namespace Volt
{
	struct PhysicsMaterialSerializationData
	{
		float staticFriction;
		float dynamicFriction;
		float bounciness;
	};

	void PhysicsMaterialSerializer::Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		AssetReference<PhysicsMaterialAsset> material = asset.ConvertTo<PhysicsMaterialAsset>();
		ScopedAssetReferenceLock materialLock{ material };

		BinaryStreamWriter streamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(*metadata, asset->GetVersion(), streamWriter);

		PhysicsMaterialSerializationData serializationData{};
		serializationData.staticFriction = material->m_material->GetStaticFriction();
		serializationData.dynamicFriction = material->m_material->GetDynamicFriction();
		serializationData.bounciness = material->m_material->GetBounciness();

		streamWriter.Write(serializationData);

		const auto filePath = g_assetManager->GetFilesystemPath(metadata->filepath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool PhysicsMaterialSerializer::Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset> destinationAsset) const
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

		PhysicsMaterialSerializationData serializationData{};
		streamReader.Read(serializationData);

		AssetReference<PhysicsMaterialAsset> physicsMat = destinationAsset.ConvertTo<PhysicsMaterialAsset>();
		ScopedAssetReferenceLock materialLock{ physicsMat };

		physicsMat->GetMaterial()->SetStaticFriction(serializationData.staticFriction);
		physicsMat->GetMaterial()->SetDynamicFriction(serializationData.dynamicFriction);
		physicsMat->GetMaterial()->SetBounciness(serializationData.bounciness);

		return true;
	}
}
