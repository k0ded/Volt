#pragma once

#include <Volt-Application/ApplicationLayer.h>

#include <EventSystem/EventListener.h>
#include <EventSystem/ApplicationEvents.h>

namespace Volt
{
	class Scene;
	class SceneRenderer;
	struct SceneRendererSettings;

	class AppRenderEvent;
	class WindowResizeEvent;

	class OnSceneLoadedEvent;
}

class GameLayer : public Volt::ApplicationLayer, public Volt::EventListener
{
public:
	GameLayer() = default;
	~GameLayer() override = default;

	void OnAttach() override;
	void OnDetach() override;

private:
	bool OnUpdateEvent(Volt::AppUpdateEvent& e);
	bool OnRenderEvent(Volt::AppRenderEvent& e);
	bool OnWindowResizeEvent(Volt::WindowResizeEvent& e);
};
