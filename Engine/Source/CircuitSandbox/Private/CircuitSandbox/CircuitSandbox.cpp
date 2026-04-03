#include "csbpch.h"
#include "CircuitSandbox.h"

#include "CircuitSandbox/Widgets/ViewportWidget.h"
#include "CircuitSandbox/Widgets/SceneViewWidget.h"
#include "CircuitSandbox/Widgets/AssetBrowserWidget.h"
#include "CircuitSandbox/Widgets/InspectorWidget.h"

#include <InputModule/Input.h>
#include <WindowModule/WindowManager_New.h>
#include <WindowModule/Window_New.h>

#include <SubSystem/SubSystemManager.h>

#include <Circuit/CircuitManager.h>
#include <Circuit/Window/CircuitWindow.h>
#include <Circuit/Widgets/SliderWidget.h>

#include <Volt-Renderer/SceneRenderer.h>
#include <Volt-Renderer/Camera/Camera.h>

#include <Volt-Application/BaseApplication.h>

#include <Volt-Scene/Scene.h>
#include <Volt-Scene/SceneManager.h>
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
}


void CircuitSandbox::OnAttach()
{
	RegisterEventListeners();
	Circuit::CircuitManager::Initialize();

	constexpr float fov = glm::radians(60.f);
	constexpr float nearPlane = 1.f;
	constexpr float farPlane = 100000.f;
	m_camera = CreateRef<Volt::Camera>(fov, 16.f / 9.f, nearPlane, farPlane);
	m_camera->SetRotation(glm::radians(glm::vec3(45.f, 135.f, 0.f)));

	const glm::vec3 startPosition = { 500.f, 500.f, 500.f };
	const float focalDistance = glm::distance(startPosition, { 0,0,0 });
	const glm::vec3 pos = -1.f * m_camera->GetForward() * focalDistance;
	m_camera->SetPosition(pos);

	SetupNewSceneData();

	//create main window
	{
		Ref<Circuit::LayoutWidget> topRow = CreateWidget(Circuit::LayoutWidget).Orientation(Circuit::LayoutOrientation::Horizontal);
		topRow->AddFlexibleSlice(
		CreateWidget(SceneViewWidget)
		.Scene(m_sceneContainer->GetScene())
		);
		topRow->AddFlexibleSlice(
		CreateWidget(ViewportWidget)
		.SceneRenderer(m_sceneRenderer)
		.Scene(m_sceneContainer->GetScene())
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

		Weak<Circuit::CircuitWindow> mainWindow = Circuit::CircuitManager::Get().CreateWindow(rootLayout);

		if (!mainWindow.IsExpired())
		{
			Volt::WindowHandle windowHandle = mainWindow.Lock()->GetWindowHandle();
			Volt::Window_New& window = Volt::WindowManager_New::Get().GetWindow(windowHandle);

			window.GetOnWindowClosed().AddLambda([](Volt::Window_New&) 
			{
				Volt::BaseApplication::Get().Quit();
			});
		}
	}

	m_isInitialized = true;
}

void CircuitSandbox::OnDetach()
{
	m_isInitialized = false;

	Circuit::CircuitManager::Shutdown();

	m_sceneRenderer = nullptr;
	m_camera = nullptr;
	s_instance = nullptr;
}

bool CircuitSandbox::OnUpdateEvent(Volt::AppUpdateEvent& e)
{
	VT_PROFILE_FUNCTION();

	return false;
}

void CircuitSandbox::SetupNewSceneData()
{
	// Scene Renderers
	{
		Volt::SceneRendererInitializer spec{};

		spec.debugName = "Editor Viewport";
		spec.drawDebug = true;

		m_sceneContainer = SubSystemManager::GetSubSystem<Volt::SceneManager>()->CreateMemoryScene("TestScene");
		m_sceneRenderer = m_sceneContainer->AttachSceneRenderer(spec);
		m_sceneRenderer->SetCamera(m_camera);

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
