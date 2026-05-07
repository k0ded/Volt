#include "vspch.h"

#include "Volt-Scene/SceneManager.h"
#include "Volt-Scene/Scene.h"

#include <Volt-Renderer/SceneRenderer.h>

#include <AssetSystem/AssetManager.h>

VT_DECLARE_LOG_CATEGORY(LogSceneManager, LogVerbosity::Trace);
VT_DEFINE_LOG_CATEGORY(LogSceneManager);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(SceneManager, Default, Engine);

	SceneManager::SceneManager()
	{
	}

	SceneManager::~SceneManager()
	{
		for (SceneContainer* sceneContainer : m_sceneContainers)
		{
			m_sceneContainerAllocator.Free(sceneContainer);
		}
	}

	SceneContainer* SceneManager::LoadScene(AssetHandle handle)
	{
		for (SceneContainer* container : m_sceneContainers)
		{
			if (container->GetScene()->GetAssetHandle() == handle)
			{
				return container;
			}
		}

		AssetReference<Scene> newScene;

		bool result = g_assetManager->TryGetAsset<Scene>(handle, newScene);
		if (!result)
		{
			VT_LOGC(Error, LogSceneManager, "Failed to load scene with handle '{}'!", handle);
			return nullptr;
		}

		SceneContainer* newContainer = m_sceneContainerAllocator.Allocate(newScene);
		m_sceneContainers.emplace_back(newContainer);

		return newContainer;
	}

	SceneContainer* SceneManager::CreateMemoryScene(StringView name)
	{
		AssetReference<Scene> newScene = Scene::CreateDefaultScene(String(name), true, true);
		
		SceneContainer* newContainer = m_sceneContainerAllocator.Allocate(newScene);
		m_sceneContainers.emplace_back(newContainer);

		return newContainer;
	}

	SceneContainer* SceneManager::CreateScene(StringView name)
	{
		AssetReference<Scene> newScene = Scene::CreateDefaultScene(String(name), true);

		SceneContainer* newContainer = m_sceneContainerAllocator.Allocate(newScene);
		m_sceneContainers.emplace_back(newContainer);

		return newContainer;
	}

	void SceneManager::UnloadAndFreeScene(SceneContainer* scene)
	{
		m_sceneContainers.erase_first_unsorted(scene);
		m_sceneContainerAllocator.Free(scene);
	}

	void SceneManager::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{

	}

	SceneContainer::SceneContainer(AssetReference<Scene> scene)
		: m_scene(scene)
	{}

	Ref<SceneRenderer> SceneContainer::AttachSceneRenderer(const SceneRendererInitializer& initializer)
	{
		Ref<SceneRenderer> newSceneRenderer = CreateRef<SceneRenderer>(initializer, m_scene->GetRenderScene());
		m_sceneRenderers.emplace_back(newSceneRenderer);

		return newSceneRenderer;
	}

	Ref<RenderScene> SceneContainer::GetRenderScene() const
	{
		return m_scene->GetRenderScene();
	}
}
