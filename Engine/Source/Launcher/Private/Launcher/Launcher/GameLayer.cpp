#include "Launcher/GameLayer.h"

#include <Volt-Scene/Scene.h>
#include <Volt-Scene/SceneManager.h>
#include <Volt-Scene/SceneEvents.h>

#include <Volt-Renderer/SceneRenderer.h>

#include <Volt-Core/Project/ProjectManager.h>

#include <AssetSystem/AssetManager.h>
#include <Navigation/Core/NavigationSystem.h>

#include <WindowModule/Events/WindowEvents.h>
#include <WindowModule/WindowManager.h>
#include <EventSystem/EventSystem.h>

void GameLayer::OnAttach()
{
	RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(GameLayer::OnUpdateEvent));
	RegisterListener<Volt::AppRenderEvent>(VT_BIND_EVENT_FN(GameLayer::OnRenderEvent));
	RegisterListener<Volt::WindowResizeEvent>(VT_BIND_EVENT_FN(GameLayer::OnWindowResizeEvent));
}

void GameLayer::OnDetach()
{
	m_scene->OnRuntimeEnd();

	m_sceneRenderer = nullptr;
	m_scene = nullptr;
}

void GameLayer::LoadStartScene()
{
	Volt::SceneManager::SetActiveScene(m_scene);

	Volt::OnSceneLoadedEvent loadEvent{ m_scene };
	Volt::EventSystem::DispatchEvent(loadEvent);

	m_scene->OnRuntimeStart();
	Volt::OnScenePlayEvent playEvent{};
	Volt::EventSystem::DispatchEvent(playEvent);

}

bool GameLayer::OnUpdateEvent(Volt::AppUpdateEvent& e)
{
	return false;
}

bool GameLayer::OnRenderEvent(Volt::AppRenderEvent& e)
{
	return false;
}

bool GameLayer::OnWindowResizeEvent(Volt::WindowResizeEvent& e)
{
	Volt::ViewportResizeEvent resizeEvent{ Volt::WindowManager::Get().GetMainWindow(), e.GetX(), e.GetY(), e.GetWidth(), e.GetHeight() };
	Volt::EventSystem::DispatchEvent(resizeEvent);

	return false;
}
