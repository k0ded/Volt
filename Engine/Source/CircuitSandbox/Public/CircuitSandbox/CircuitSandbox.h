#pragma once

#include <Volt-Application/ApplicationLayer.h>

#include <WindowModule/WindowHandle.h>

#include <EventSystem/EventListener.h>

#include <AssetSystem/AssetReference.h>
#include <Volt-Scene/Scene.h>

namespace Volt
{
	class Event;
	class AppUpdateEvent;
	class AppRenderEvent;
	class KeyPressedEvent;

	class SceneRenderer;
	class SceneContainer;
	class Camera;
}
class OutlineSceneRendererExtension;
class ObjectIDSceneRendererExtension;
class DebugSceneRendererExtension;

class CircuitSandbox : public Volt::ApplicationLayer, public Volt::EventListener
{
public:
	CircuitSandbox();
	~CircuitSandbox() override;

	void OnAttach() override;
	void OnDetach() override;

	VT_NODISCARD VT_INLINE static CircuitSandbox& Get() { return *s_instance; }
private:
	void RegisterEventListeners();

	bool OnUpdateEvent(Volt::AppUpdateEvent& e);
	
	bool m_isInitialized = false;

	//temp
	void SetupNewSceneData();
	Ref<Volt::SceneRenderer> m_sceneRenderer;
	Volt::SceneContainer* m_sceneContainer;

	//Ref<OutlineSceneRendererExtension> m_outlineSceneRendererExtension;
	//Ref<ObjectIDSceneRendererExtension> m_objectIDSceneRendererExtension;
	//Ref<DebugSceneRendererExtension> m_debugSceneRendererExtension;

	Ref<Volt::Camera> m_camera;
	Volt::WindowHandle m_window;
	//end temp


	inline static CircuitSandbox* s_instance = nullptr;
};
