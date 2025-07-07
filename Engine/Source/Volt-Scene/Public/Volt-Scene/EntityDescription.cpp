#include "vspch.h"
#include "EntityDescription.h"

Volt::EntityDesc::EntityDesc(EntityID entityID, AssetHandle sceneHandle)
	:m_sceneHandle(sceneHandle), m_entityID(entityID)
{}
