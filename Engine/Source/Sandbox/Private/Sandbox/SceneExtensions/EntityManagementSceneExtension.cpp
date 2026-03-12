#include "sbpch.h"

#include "Sandbox/SceneExtensions/EntityManagementSceneExtension.h"
#include "Sandbox/DirtyAssetsManager.h"

#include <Volt-Scene/Scene.h>

void EntityManagementSceneExtension::OnEntityCreated(Volt::Entity entity)
{
	if (m_scene->IsFlagSet(Volt::AssetFlag::MemoryOnly))
	{
		return;
	}

	if (Volt::AssetHandle entityDescHandle = m_scene->GetEntityDescHandleFromEntityID(entity.GetID()); entityDescHandle != Volt::Asset::Null())
	{
		DirtyAssetsManager::Get().MarkAssetDirty(entityDescHandle);
	}
}

void EntityManagementSceneExtension::OnEntityDestroyed(Volt::EntityID entityId)
{

}
