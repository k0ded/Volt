#pragma once

#include <EntitySystem/Entity.h>

namespace Volt
{
	class Scene;

	class SceneExtension
	{
	public:
		virtual ~SceneExtension() = default;

		virtual void OnEntityCreated(Entity entity) {}
		virtual void OnEntityDestroyed(EntityID entityId) {}

	protected:
		friend class SceneExtensionManager;

		Scene* m_scene = nullptr;
	};

	class SceneExtensionManager
	{
	public:
		SceneExtensionManager(Scene& scene);

		void OnEntityCreated(Entity entity);
		void OnEntityDestroyed(EntityID entityId);

		template<typename T, typename... Args>
		void AddExtension(Args&&... args)
		{
			Ref<T> sceneExt = CreateRef<T>(std::forward<Args>(args)...);
			sceneExt->m_scene = &m_scene;

			m_sceneExtensions.emplace_back(sceneExt);
		}

	private:
		Vector<Ref<SceneExtension>> m_sceneExtensions;
		Scene& m_scene;
	};
}
