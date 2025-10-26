#include "vspch.h"
#include "EntityDescription.h"

#include "Volt-Scene/EntityDescCustomMetadata.h"

namespace Volt
{
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
}
