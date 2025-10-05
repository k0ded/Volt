#pragma once

#include <Volt-Application/ApplicationLayer.h>

#include <EventSystem/EventListener.h>
#include <EventSystem/ApplicationEvents.h>

#include <WindowModule/Events/WindowEvents.h>

namespace Volt
{
	class Scene;
	class SceneRenderer;
	class Camera;
}

class TestingLayer : public Volt::ApplicationLayer, public Volt::EventListener
{
public:
	TestingLayer() = default;
	~TestingLayer() override = default;

	void OnAttach() override;
	void OnDetach() override;

private:
	bool OnUpdateEvent(Volt::AppUpdateEvent& e);
	bool OnRenderEvent(Volt::AppRenderEvent& e);
	bool OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e);
	bool OnWindowResizeEvent(Volt::WindowResizeEvent& e);

	Ref<Volt::Scene> m_scene;
	Ref<Volt::SceneRenderer> m_sceneRenderer;
	Ref<Volt::Camera> m_camera;
};
