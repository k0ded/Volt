#include "vspch.h"

#include "Volt-Scene/SceneExtension.h"

namespace Volt
{
	SceneExtensionManager::SceneExtensionManager(Scene& scene)
		: m_scene(scene)
	{

	}

	void SceneExtensionManager::OnEntityCreated(Entity entity)
	{
		for (const Ref<SceneExtension>& sceneExt : m_sceneExtensions)
		{
			sceneExt->OnEntityCreated(entity);
		}
	}

	void SceneExtensionManager::OnEntityDestroyed(EntityID entityId)
	{
		for (const Ref<SceneExtension>& sceneExt : m_sceneExtensions)
		{
			sceneExt->OnEntityDestroyed(entityId);
		}
	}
}
