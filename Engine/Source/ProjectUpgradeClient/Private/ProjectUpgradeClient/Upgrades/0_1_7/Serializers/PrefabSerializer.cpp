#include "ProjectUpgradeClient/Upgrades/0_1_7/Serializers/PrefabSerializer.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/Serializers/SceneSerializer.h"
#include "ProjectUpgradeClient/Upgrades/0_1_7/Serializers/EntityDescriptionSerializer.h"
#include "ProjectUpgradeClient/Common/YAMLMemoryStreamReader.h"
#include "ProjectUpgradeClient/Common/YAMLMemoryStreamWriter.h"
#include "ProjectUpgradeClient/Common/CommonSerializeFuncs.h"

#define private public
#include <Volt-Scene/Prefab.h>
#undef private
#include <Volt-Scene/Scene.h>

#include <AssetSystem/AssetManager.h>

namespace Volt
{
	PrefabSerializer::PrefabSerializer()
	{
		s_instance = this;
	}

	PrefabSerializer::~PrefabSerializer()
	{
		s_instance = nullptr;
	}

	void PrefabSerializer::Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
	}

	bool PrefabSerializer::Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const
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

		Buffer buffer{};
		streamReader.Read(buffer);

		YAMLMemoryStreamReader yamlStreamReader{};
		if (!yamlStreamReader.ConsumeBuffer(buffer))
		{
			destinationAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		AssetReference<Prefab> prefab = destinationAsset.ConvertTo<Prefab>();

		AssetReference<Scene> prefabScene = g_assetManager->CreateAnonymousAsset<Scene>("PrefabScene");

		yamlStreamReader.EnterScope("Prefab");
		{
			prefab->m_version = yamlStreamReader.ReadAtKey("version", uint32_t(0));
			prefab->m_rootEntityId = yamlStreamReader.ReadAtKey("rootEntityId", Entity::NullID());

			yamlStreamReader.ForEach("Entities", [&]()
			{
				EntityDescSerializer::Get().DeserializeEntity(prefabScene, yamlStreamReader);
			});

			yamlStreamReader.ForEach("PrefabReferences", [&]()
			{
				EntityID entityId = yamlStreamReader.ReadAtKey("entity", Entity::NullID());
				AssetHandle prefabHandle = yamlStreamReader.ReadAtKey("prefabHandle", Asset::Null());
				EntityID prefabEntityReference = yamlStreamReader.ReadAtKey("prefabEntityReferences", Entity::NullID());

				auto& prefabRefData = prefab->m_prefabReferencesMap[entityId];
				prefabRefData.prefabAsset = prefabHandle;
				prefabRefData.prefabReferenceEntity = prefabEntityReference;
			});
		}
		yamlStreamReader.ExitScope();

		prefab->m_prefabScene = prefabScene;
		return true;
	}
}
