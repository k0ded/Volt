#include "ProjectUpgradeClient/Upgrades/0_1_7/Serializers/PhysicsMaterialSerializer.h"

#define private public
#include <Volt-Physics/PhysicsMaterialAsset.h>
#undef private

#include <AssetSystem/AssetManager.h>

#include <PhysicsInterface/PhysicsMaterial.h>

#include <FileSystemModule/Filesystem.h>

namespace Volt
{
	struct PhysicsMaterialSerializationData
	{
		float staticFriction;
		float dynamicFriction;
		float bounciness;
	};

	void PhysicsMaterialSerializer::Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		AssetReference<PhysicsMaterialAsset> material = asset.ConvertTo<PhysicsMaterialAsset>();

		BinaryStreamWriter streamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(*metadata, asset->GetVersion(), streamWriter);

		PhysicsMaterialSerializationData serializationData{};
		serializationData.staticFriction = material->m_material->GetStaticFriction();
		serializationData.dynamicFriction = material->m_material->GetDynamicFriction();
		serializationData.bounciness = material->m_material->GetBounciness();

		streamWriter.Write(serializationData);

		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool PhysicsMaterialSerializer::Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const
	{
		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);

		if (!Filesystem::Exists(filePath))
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

		physicsMat->GetMaterial()->SetStaticFriction(serializationData.staticFriction);
		physicsMat->GetMaterial()->SetDynamicFriction(serializationData.dynamicFriction);
		physicsMat->GetMaterial()->SetBounciness(serializationData.bounciness);

		return true;
	}
}
