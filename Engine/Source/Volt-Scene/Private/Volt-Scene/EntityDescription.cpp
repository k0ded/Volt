#include "vspch.h"
#include "EntityDescription.h"

#include "Volt-Scene/EntityDescCustomMetadata.h"
#include "Volt-Scene/EntityDescSerialization.h"
#include "Volt-Scene/Scene.h"

#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::EntityDesc, EntityDesc);

	EntityDesc::EntityDesc(EntityID entityID, AssetHandle sceneHandle)
		:m_sceneHandle(sceneHandle), m_entityID(entityID)
	{}

	void EntityDesc::SetupInitialCustomMetadata(CustomAssetMetadataVector& customMetadata)
	{
		size_t newSize = sizeof(EntityDescCustomMetadata);
		customMetadata.resize(newSize);
		EntityDescCustomMetadata& entityDescCustomMeta = reinterpret_cast<EntityDescCustomMetadata&>(*customMetadata.data());
		entityDescCustomMeta.sceneHandle = m_sceneHandle;
		entityDescCustomMeta.entityID = m_entityID;
	}

	void EntityDesc::Serialize(Archive& archive)
	{
		if (!archive.IsLoading())
		{
			ReadOnlyAssetMetadata ownerSceneMetadata = g_assetManager->GetReadOnlyAssetMetadata(m_sceneHandle);

			//if the scene is not loaded here, the entity is not supposed to be loaded, and cannot be saved
			VT_ENSURE(ownerSceneMetadata->IsLoaded());
			//if the scene is a memory asset it doesnt have a path yet, and will thus fail the save of this entity
			VT_ENSURE(!ownerSceneMetadata->IsMemoryAsset());

			AssetReference<Scene> scene = g_assetManager->GetAssetImmediately<Scene>(m_sceneHandle);
			ScopedAssetReferenceLock sceneLock{ scene };

			EntityDescSerialization::SerializeEntity(archive, scene->GetEntityFromID(m_entityID));
		}
		else
		{
			EntityDescSerialization::DeserializeEntityData(archive, m_entitySerializationData);
		}
	}
}
