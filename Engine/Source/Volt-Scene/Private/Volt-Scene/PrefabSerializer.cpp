#include "vspch.h"
#include "Volt-Scene/PrefabSerializer.h"
#include "Volt-Scene/Prefab.h"
#include "Volt-Scene/SceneSerializer.h"
#include "Volt-Scene/Scene.h"

#include <AssetSystem/AssetManager_New.h>

#include <Volt-Scene/EntityDescriptionSerializer.h>

#include <CoreUtilities/FileIO/YAMLMemoryStreamWriter.h>
#include <CoreUtilities/FileIO/YAMLMemoryStreamReader.h>
#include <AssetSystem/AssetLocks.h>

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

	void PrefabSerializer::Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset_New>& asset) const
	{
		const AssetReference<Prefab> prefab = asset.ConvertTo<Prefab>();
		ScopedAssetReferenceLock prefabLock{ prefab };

		YAMLMemoryStreamWriter yamlStreamWriter{};
		yamlStreamWriter.BeginMap();
		yamlStreamWriter.BeginMapNamned("Prefab");

		yamlStreamWriter.SetKey("version", prefab->m_version);
		yamlStreamWriter.SetKey("rootEntityId", prefab->m_rootEntityId);

		yamlStreamWriter.BeginSequence("Entities");
		{
			for (const auto entity : prefab->m_prefabScene->GetAllEntities())
			{
				EntityDescSerializer::Get().SerializeEntity(entity, yamlStreamWriter);
			}
		}
		yamlStreamWriter.EndSequence();

		yamlStreamWriter.BeginSequence("PrefabReferences");
		{
			for (const auto& ref : prefab->m_prefabReferencesMap)
			{
				yamlStreamWriter.BeginMap();
				yamlStreamWriter.SetKey("entity", ref.first);
				yamlStreamWriter.SetKey("prefabHandle", ref.second.prefabAsset);
				yamlStreamWriter.SetKey("prefabEntityReference", ref.second.prefabReferenceEntity);
				yamlStreamWriter.EndMap();
			}
		}
		yamlStreamWriter.EndSequence();

		yamlStreamWriter.EndMap();
		yamlStreamWriter.EndMap();

		BinaryStreamWriter streamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(*metadata, asset->GetVersion(), streamWriter);

		Buffer buffer = yamlStreamWriter.WriteAndGetBuffer();
		streamWriter.Write(buffer);
		buffer.Release();

		const auto filePath = g_assetManager->GetFilesystemPath(metadata->filepath);
		const auto directory = filePath.parent_path();
		if (!std::filesystem::exists(directory))
		{
			std::filesystem::create_directories(directory);
		}
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool PrefabSerializer::Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset_New> destinationAsset) const
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

		Buffer buffer{};
		streamReader.Read(buffer);

		YAMLMemoryStreamReader yamlStreamReader{};
		if (!yamlStreamReader.ConsumeBuffer(buffer))
		{
			destinationAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		AssetReference<Prefab> prefab = destinationAsset.ConvertTo<Prefab>();
		ScopedAssetReferenceLock prefabLock{ prefab };

		Ref<Scene> prefabScene = CreateRef<Scene>();

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
