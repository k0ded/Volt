#include "csbpch.h"
#include "CircuitSandbox.h"

#include "CircuitSandbox/Widgets/ViewportWidget.h"
#include "CircuitSandbox/Widgets/SceneViewWidget.h"
#include "CircuitSandbox/Widgets/AssetBrowserWidget.h"
#include "CircuitSandbox/Widgets/InspectorWidget.h"

#include <InputModule/Input.h>
#include <InputModule/InputCodes.h>
#include <InputModule/Events/KeyboardEvents.h>

#include <SubSystem/SubSystemManager.h>

#include <WindowModule/Events/WindowEvents.h>
#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <Circuit/CircuitManager.h>
#include <Circuit/Widgets/SliderWidget.h>

#include <Volt-Renderer/SceneRenderer.h>
#include <Volt-Renderer/Camera/Camera.h>

#include <Volt-Scene/Scene.h>
#include <Circuit/Widgets/Layout/LayoutWidget.h>

CircuitSandbox::CircuitSandbox()
{
	VT_ASSERT_MSG(!s_instance, "CircuitSandbox already exists!");
	s_instance = this;
}

CircuitSandbox::~CircuitSandbox()
{
	s_instance = nullptr;
}

void CircuitSandbox::RegisterEventListeners()
{
	auto isInitializedPred = [this]() { return m_isInitialized; };

	RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(CircuitSandbox::OnUpdateEvent), isInitializedPred);
	RegisterListener<Volt::AppRenderEvent>(VT_BIND_EVENT_FN(CircuitSandbox::OnRenderEvent), isInitializedPred);
	RegisterListener<Volt::KeyPressedEvent>(VT_BIND_EVENT_FN(CircuitSandbox::OnKeyPressedEvent), isInitializedPred);
}


void CircuitSandbox::OnAttach()
{
	RegisterEventListeners();

	//Volt::WindowManager::Get().GetMainWindow().Maximize();



	m_editorScene = Volt::Scene::CreateDefaultScene("New Scene", true);
	SetupNewSceneData();

	Ref<Circuit::LayoutWidget> topRow = CreateWidget(Circuit::LayoutWidget).Orientation(Circuit::LayoutOrientation::Horizontal);
	topRow->AddFlexibleSlice(
	CreateWidget(SceneViewWidget)
	);
	topRow->AddFlexibleSlice(
	CreateWidget(ViewportWidget)
	.SceneRenderer(m_sceneRenderer)
	);
	topRow->AddFlexibleSlice(
	CreateWidget(InspectorWidget)
	);

	Ref<Circuit::LayoutWidget> rootLayout = CreateWidget(Circuit::LayoutWidget).Orientation(Circuit::LayoutOrientation::Vertical);
	rootLayout->AddFlexibleSlice(topRow);
	rootLayout->AddFixedSlice(
	CreateWidget(AssetBrowserWidget),
	300.f
	);

	Circuit::CircuitManager::Initialize(rootLayout);

	constexpr float fov = glm::radians(60.f);
	constexpr float nearPlane = 1.f;
	constexpr float farPlane = 100000.f;
	m_camera = CreateRef<Volt::Camera>(fov, 16.f / 9.f, nearPlane, farPlane);
	m_camera->SetRotation(glm::radians(glm::vec3(45.f, 135.f, 0.f)));

	const glm::vec3 startPosition = { 500.f, 500.f, 500.f };
	const float focalDistance = glm::distance(startPosition, { 0,0,0 });
	const glm::vec3 pos = -1.f * m_camera->GetForward() * focalDistance;
	m_camera->SetPosition(pos);



	m_isInitialized = true;
}

void CircuitSandbox::OnDetach()
{
	m_isInitialized = false;

	Circuit::CircuitManager::Shutdown();

	m_editorScene = nullptr;
	m_sceneRenderer = nullptr;
	m_camera = nullptr;

	s_instance = nullptr;
}

bool CircuitSandbox::OnUpdateEvent(Volt::AppUpdateEvent& e)
{
	VT_PROFILE_FUNCTION();

	return false;
}

bool CircuitSandbox::OnRenderEvent(Volt::AppRenderEvent& e)
{
	VT_PROFILE_FUNCTION();

	if (m_sceneRenderer)
	{
		m_sceneRenderer->OnRenderEditor(m_camera, e.GetTimestep());
	}


	return false;
}

bool CircuitSandbox::OnKeyPressedEvent(Volt::KeyPressedEvent& e)
{

	return false;
}

void CircuitSandbox::SetupNewSceneData()
{
	// Scene Renderers
	{
		Volt::SceneRendererCreateInfo spec{};

		spec.debugName = "Editor Viewport";
		spec.renderScene = m_editorScene->GetRenderScene();
		spec.drawDebug = true;

		if (m_sceneRenderer)
		{
			spec.initialResolution = { m_sceneRenderer->GetFinalImage()->GetWidth(), m_sceneRenderer->GetFinalImage()->GetHeight() };
		}

		m_sceneRenderer = CreateRef<Volt::SceneRenderer>(spec);
		/*auto gridExt = m_sceneRenderer->AddExtension<GridSceneRendererExtension>(Volt::SceneRendererExtensionStage::PostPostProcessing);
		gridExt->GetIsEnabledDelegate().BindLambda([]()
		{
			return UserSettingsManager::GetSettings().sceneSettings.gridEnabled;
		});

		m_outlineSceneRendererExtension = m_sceneRenderer->AddExtension<OutlineSceneRendererExtension>(Volt::SceneRendererExtensionStage::PostPostProcessing);
		m_objectIDSceneRendererExtension = m_sceneRenderer->AddExtension<ObjectIDSceneRendererExtension>(Volt::SceneRendererExtensionStage::PreGBuffer);
		m_debugSceneRendererExtension = m_sceneRenderer->AddExtension<DebugSceneRendererExtension>(Volt::SceneRendererExtensionStage::PostPostProcessing, m_debugRenderer);*/
	}
}
