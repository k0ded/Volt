#include "sbpch.h"
#include "Sandbox/Sandbox_New.h"
#include "Sandbox/Camera/EditorCameraController.h"

#include <Volt-Renderer/SceneRenderer.h>
#include <Volt-Renderer/Camera/Camera.h>

#include <Volt-Scene/Scene.h>

#include <EventSystem/ApplicationEvents.h>
#include <InputModule/Events/KeyboardEvents.h>

Sandbox_New::Sandbox_New()
{}

Sandbox_New::~Sandbox_New()
{}

void Sandbox_New::OnAttach()
{
	RegisterEventListeners();

	m_editorCameraController = CreateRef<EditorCameraController>(glm::radians(60.f), 1.f, 100000.f);

	m_runtimeScene = Volt::Scene::CreateDefaultScene("Test");

	Volt::SceneRendererCreateInfo createInfo{};
	createInfo.renderScene = m_runtimeScene->GetRenderScene();
	createInfo.drawDebug = true;

	m_sceneRenderer = CreateRef<Volt::SceneRenderer>(createInfo);
	m_camera = CreateRef<Volt::Camera>(glm::radians(60.f), 16.f / 9.f, 0.1f, 100000.f);

	m_isInitialized = true;
}

void Sandbox_New::OnDetach()
{
	m_isInitialized = false;
}

void Sandbox_New::RegisterEventListeners()
{
	auto isInitializedPred = [this]() { return m_isInitialized; };

	RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(Sandbox_New::OnUpdateEvent), isInitializedPred);
	RegisterListener<Volt::AppImGuiUpdateEvent>(VT_BIND_EVENT_FN(Sandbox_New::OnImGuiUpdateEvent), isInitializedPred);
	RegisterListener<Volt::AppRenderEvent>(VT_BIND_EVENT_FN(Sandbox_New::OnRenderEvent), isInitializedPred);
	RegisterListener<Volt::KeyPressedEvent>(VT_BIND_EVENT_FN(Sandbox_New::OnKeyPressedEvent), isInitializedPred);
}

bool Sandbox_New::OnUpdateEvent(Volt::AppUpdateEvent& e)
{
	return false;
}

bool Sandbox_New::OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e)
{
	return false;
}

bool Sandbox_New::OnRenderEvent(Volt::AppRenderEvent& e)
{
	m_sceneRenderer->OnRenderEditor(m_camera, e.GetTimestep());
	return false;
}

bool Sandbox_New::OnKeyPressedEvent(Volt::KeyPressedEvent& e)
{
	return false;
}
