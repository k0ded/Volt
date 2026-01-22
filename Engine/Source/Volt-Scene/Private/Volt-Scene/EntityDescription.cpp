#include "vspch.h"
#include "EntityDescription.h"

#include "Volt-Scene/EntityDescCustomMetadata.h"
#include "Volt-Scene/EntityDescSerialization.h"
#include "Volt-Scene/Scene.h"

#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::EntityDesc, EntityDesc);
	VT_REGISTER_CUSTOM_ASSET_METADATA_TYPE(EntityDescCustomMetadata, AssetTypes::EntityDesc);

	EntityDesc::EntityDesc(EntityID entityID, AssetHandle sceneHandle)
		: m_sceneHandle(sceneHandle), m_entityID(entityID)
	{}

	void EntityDesc::OnPreSave(CustomAssetMetadata& customMetadata)
	{
		EntityDescCustomMetadata& entityDescCustomMeta = customMetadata.GetMutableCustomMetadata<EntityDescCustomMetadata>();
		entityDescCustomMeta.sceneHandle = m_sceneHandle;
		entityDescCustomMeta.entityID = m_entityID;
	}

	void EntityDesc::Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata)
	{
		archive.UseVersion(EntityDescSerialization::EntityDescArchiveVersion::guid);
		//const size_t entityDescVersion = archive.GetVersion(EntityDescSerialization::EntityDescArchiveVersion::guid);

		EntityDescSerialization::SerializationData entityData;

		//if SAVING the entity desc, fill the serializationData
		if (!archive.IsLoading())
		{
			entityData.entityId = m_entityID;
			entityData.ownerSceneAssetHandle = m_sceneHandle;
			entityData.components = m_componentData;
		}

		EntityDescSerialization::SerializeEntityDescData(archive, entityData);

		//if LOADING the entity desc, apply the loaded serialization data to this asset
		if (archive.IsLoading())
		{
			m_entityID = entityData.entityId;
			m_sceneHandle = entityData.ownerSceneAssetHandle;
			m_componentData = std::move(entityData.components);

			//if a header has a component size == 0, the component is an old version and the sizes need to be adjusted according to a sorted order
			if (!m_componentData.headers.empty())
			{
				if (m_componentData.headers[0].componentDataSize == 0)
				{
					VT_ENSURE(m_componentData.headers[0].componentDataOffset == 0);

					for (int i = 0; i < m_componentData.headers.size() - 1; i++)
					{
						EntityDescSerialization::ComponentHeader& header = m_componentData.headers[i];
						EntityDescSerialization::ComponentHeader& nextHeader = m_componentData.headers[i + 1];
						header.componentDataSize = nextHeader.componentDataOffset - header.componentDataOffset;
					}
					EntityDescSerialization::ComponentHeader& lastHeader = m_componentData.headers[m_componentData.headers.size() - 1];
					lastHeader.componentDataSize = m_componentData.data.size() - lastHeader.componentDataOffset;
				}
			}

		}



		////SAVING
		//if (!archive.IsLoading())
		//{
		//	// Either use the assigned owner scene, or get it from the asset handle.
		//	// Need to rethink this at some point. #Scene_TODO
		//	AssetReference<Scene> sceneReference;

		//	if (m_ownerScene.IsValid())
		//	{
		//		sceneReference = m_ownerScene;
		//	}
		//	else
		//	{
		//		ReadOnlyAssetMetadata ownerSceneMetadata = g_assetManager->GetReadOnlyAssetMetadata(m_sceneHandle);

		//		//if the scene is not loaded here, the entity is not supposed to be loaded, and cannot be saved
		//		VT_ENSURE(ownerSceneMetadata->IsLoaded());
		//		VT_ENSURE(ownerSceneMetadata->HasFilepath());

		//		sceneReference = g_assetManager->GetAssetImmediately<Scene>(m_sceneHandle);
		//	}

		//	//if the scene is a memory asset it doesnt have a path yet, and will thus fail the save of this entity
		//	VT_ENSURE(!sceneReference->IsFlagSet(AssetFlag::MemoryOnly));

		//	EntityDescSerialization::SerializeEntity(archive, sceneReference->GetEntityFromID(m_entityID), m_sceneHandle);
		//}
		////LOADING
		//else
		//{
		//	EntityDescSerialization::DeserializeEntityData(archive, m_entitySerializationData);
		//	m_sceneHandle = m_entitySerializationData.ownerSceneAssetHandle;
		//	m_entityID = m_entitySerializationData.entityId;
		//	if (entityDescVersion < EntityDescSerialization::EntityDescArchiveVersion::AllowModifyingPartsOfSavedata)
		//	{
		//	}
		//}
	}

	bool EntityDesc::UpdateComponentData()
	{
		VT_PROFILE_FUNCTION();
		AssetReference<Scene> sceneReference;
		g_assetManager->TryGetAssetIfLoaded(m_sceneHandle, sceneReference);


		if (!sceneReference.IsValid())
		{
			return false;
		}

		if (!sceneReference->IsEntityValid(m_entityID))
		{
			return false;
		}

		Entity entity = sceneReference->GetEntityFromID(m_entityID);

		EntityDescSerialization::GatherComponentData(entity, m_componentData);
		return true;
	}
	bool EntityDesc::UpdateComponentData(VoltGUID changedComponent)
	{
		VT_PROFILE_FUNCTION();
		AssetReference<Scene> sceneReference;
		g_assetManager->TryGetAssetIfLoaded(m_sceneHandle, sceneReference);

		if (!sceneReference.IsValid())
		{
			return false;
		}

		if (!sceneReference->IsEntityValid(m_entityID))
		{
			return false;
		}

		Entity entity = sceneReference->GetEntityFromID(m_entityID);

		EntityDescSerialization::UpdateSingleComponentInData(entity, changedComponent, m_componentData);

		return true;
	}
}
