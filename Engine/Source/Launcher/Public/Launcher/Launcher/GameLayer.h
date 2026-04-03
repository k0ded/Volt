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

	class SceneContainer;
	class SceneManager;
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

	void RenderWindow(Volt::Window_New& window);

	Ref<Volt::Camera> m_camera;
	
	Volt::SceneManager* m_sceneManager = nullptr;
	Volt::SceneContainer* m_sceneContainer = nullptr;
	Ref<Volt::SceneRenderer> m_sceneRenderer;

	Volt::WindowHandle m_window;
};
