#pragma once

#include "Volt-Scene/Config.h"
#include "Volt-Scene/Scene.h"

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <AssetSystem/AssetReference.h>

#include <EventSystem/EventListener.h>

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>
#include <CoreUtilities/Containers/ArrayView.h>

namespace Volt
{
	class SceneRenderer;
	class RenderScene;

	class AppRenderEvent;

	struct SceneRendererInitializer;

	class SceneContainer
	{
	public:
		SceneContainer(AssetReference<Scene> scene);

		VTS_API Ref<SceneRenderer> AttachSceneRenderer(const SceneRendererInitializer& initializer);
		
		VTS_API Ref<RenderScene> GetRenderScene() const;

		VT_INLINE ArrayView<Ref<SceneRenderer>> GetSceneRenderers() const { return m_sceneRenderers; }
		VT_INLINE AssetReference<Scene> GetScene() const { return m_scene; }

	private:
		AssetReference<Scene> m_scene;
		Vector<Ref<SceneRenderer>> m_sceneRenderers;
	};

	class SceneManager : public SubSystem
	{
	public:
		SceneManager();
		~SceneManager();

		VTS_API SceneContainer* LoadScene(AssetHandle handle);
		VTS_API SceneContainer* CreateMemoryScene(StringView name);
		VTS_API SceneContainer* CreateScene(StringView name);

		VTS_API void UnloadAndFreeScene(SceneContainer* scene);

		VT_INLINE ArrayView<SceneContainer*> GetSceneContainers() const { return m_sceneContainers; }

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{0BF68950-4153-46C3-9738-80B8BC141EFC}"_guid);

	private:
		PagedAtomicArenaAllocator<SceneContainer, 4> m_sceneContainerAllocator;
		Vector<SceneContainer*> m_sceneContainers;
	};
}
