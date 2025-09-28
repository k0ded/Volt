#pragma once

#include <Volt-Application/ApplicationLayer.h>

#include <EventSystem/EventListener.h>

namespace Volt
{
	class SceneRenderer;
	class Scene;
	class Camera;

	class AppUpdateEvent;
	class AppImGuiUpdateEvent;
	class AppRenderEvent;
	class KeyPressedEvent;
	class ViewportResizeEvent;
}

class EditorCameraController;

class Sandbox_New : public Volt::ApplicationLayer, public Volt::EventListener
{
public:
	Sandbox_New();
	~Sandbox_New() override;

	void OnAttach() override;
	void OnDetach() override;

private:
	void RegisterEventListeners();

	bool OnUpdateEvent(Volt::AppUpdateEvent& e);
	bool OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e);
	bool OnRenderEvent(Volt::AppRenderEvent& e);
	bool OnKeyPressedEvent(Volt::KeyPressedEvent& e);

	Ref<EditorCameraController> m_editorCameraController;

	Ref<Volt::SceneRenderer> m_sceneRenderer;
	Ref<Volt::Scene> m_runtimeScene;
	Ref<Volt::Camera> m_camera;

	bool m_isInitialized = false;
};
