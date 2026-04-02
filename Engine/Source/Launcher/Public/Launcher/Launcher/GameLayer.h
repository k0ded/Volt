#pragma once

#include <Volt-Scene/Scene.h>
#include <Volt-Renderer/Camera/Camera.h>

#include <Volt-Application/ApplicationLayer.h>

#include <WindowModule/WindowHandle.h>

#include <AssetSystem/AssetReference.h>

#include <EventSystem/EventListener.h>
#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/Pointers/Ref.h>

namespace Volt
{
	class Scene;
	class SceneRenderer;
	struct SceneRendererSettings;

	class AppRenderEvent;
	class Window_New;

	class OnSceneLoadedEvent;
}

class GameLayer : public Volt::ApplicationLayer, public Volt::EventListener
{
public:
	GameLayer(Volt::WindowHandle window);
	~GameLayer() override = default;

	void OnAttach() override;
	void OnDetach() override;

private:
	bool OnUpdateEvent(Volt::AppUpdateEvent& e);
	bool OnRenderEvent(Volt::AppRenderEvent& e);

	void RenderWindow(Volt::Window_New& window);

	AssetReference<Volt::Scene> m_scene;
	Ref<Volt::SceneRenderer> m_sceneRenderer;
	Ref<Volt::Camera> m_camera;
	
	Volt::WindowHandle m_window;
};
