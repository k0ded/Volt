#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/Camera/EditorCameraController.h"

#include "Sandbox/UISystems/ModalSystem.h"

#include "Sandbox/NodeGraph/IONodeGraphEditorHelpers.h"

#include "Sandbox/Window/PropertiesPanel.h"
#include "Sandbox/Window/ViewportPanel.h"
#include "Sandbox/Window/GameViewPanel.h"
#include "Sandbox/Window/SceneViewPanel.h"
#include "Sandbox/Window/AssetBrowser/AssetBrowserPanel.h"
#include "Sandbox/Window/LogPanel.h"
#include "Sandbox/Window/EngineStatisticsPanel.h"
#include "Sandbox/Window/ThemesPanel.h"
#include "Sandbox/Window/EditorSettingsPanel.h"
#include "Sandbox/Window/PhysicsPanel.h"
#include "Sandbox/Window/RendererSettingsPanel.h"
#include "Sandbox/Window/SceneSettingsPanel.h"
#include "Sandbox/Window/WorldEnginePanel.h"
#include "Sandbox/Window/MosaicEditor/MosaicEditorPanel.h"
#include "Sandbox/Window/SkeletonEditorPanel.h"
#include "Sandbox/Window/AnimationEditorPanel.h"
#include "Sandbox/Window/GameUIEditorPanel.h"
#include "Sandbox/Window/RenderResourcesPanel.h"
#include "Sandbox/Window/RenderGraphDebuggerPanel.h"
#include "Sandbox/Window/TextureViewerPanel.h"
#include "Sandbox/Window/DebugToolsPanel.h"
#include "Sandbox/Window/ProjectConversionPanel.h"
#include "Sandbox/Window/Animation/AnimationGraphEditorPanel.h"

#include "Sandbox/SceneRendererExtensions/GridSceneRendererExtension.h"
#include "Sandbox/SceneRendererExtensions/OutlineSceneRendererExtension.h"
#include "Sandbox/SceneRendererExtensions/ObjectIDSceneRendererExtension.h"

#include "Sandbox/Modals/MeshImportModal.h"
#include "Sandbox/Modals/TextureImportModal.h"

#include "Sandbox/DirtyAssetsManager.h"
#include "Sandbox/EditorAssetManager.h"

#include "Sandbox/Utility/EditorResources.h"
#include "Sandbox/Utility/EditorLibrary.h"
#include "Sandbox/Utility/SelectionManager.h"
#include "Sandbox/Utility/NodeEditorHelpers.h"

#include "Sandbox/UserSettingsManager.h"

#include <InputModule/Input.h>
#include <InputModule/InputCodes.h>

#include <Volt-Scene/Scene.h>
#include <Volt-Scene/SceneEvents.h>
#include <Volt-Scene/EntityDescCustomMetadata.h>
#include <Volt-Scene/EntityDescSerialization.h>

#include <Volt-Renderer/Camera/Camera.h>
#include <Volt-Renderer/SceneRenderer.h>

#include <Volt-Application/UI/UIUtility.h>
#include <Volt-Application/UI/ImGuiSubSystem.h>

#include <SubSystem/SubSystemManager.h>

#include <Volt-Core/Project/ProjectManager.h>
#include <Volt-CoreComponents/RenderingComponents.h>

#include <WindowModule/Events/WindowEvents.h>
#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <NavigationEditor/Tools/NavMeshDebugDrawer.h>

#include <RHIModule/Images/Image.h>

#include <EventSystem/EventSystem.h>
#include <EventSystem/ApplicationEvents.h>

#include <EntitySystem/Entity.h>

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/Profiling/Profiling.h>

Sandbox::Sandbox()
{
	VT_ASSERT_MSG(!s_instance, "Sandbox already exists!");
	s_instance = this;
}

Sandbox::~Sandbox()
{
	s_instance = nullptr;
}

void Sandbox::OnAttach()
{
	RegisterEventListeners();

	g_editorAssetManager = CreateScope<EditorAssetManager>(*g_assetManager);

	SelectionManager::Initialize();
	EditorResources::Initialize();
	VersionControl::Initialize(VersionControlSystem::Perforce);

	NodeEditorHelpers::Initialize();
	IONodeGraphEditorHelpers::Initialize();

	SelectionManager::RegisterSelectionChangedCallback([&](const Vector<Volt::EntityID>& entities, SelectionContext context)
	{
		if (context == SelectionContext::Scene && m_outlineSceneRendererExtension)
		{
			m_outlineSceneRendererExtension->UpdateSelection(entities);
		}
	});

	//Volt::WindowManager::Get().GetMainWindow().Maximize();

	m_editorCameraController = CreateRef<EditorCameraController>(glm::radians(60.f), 1.f, 100000.f);

	UserSettingsManager::LoadUserSettings();
	const auto& userSettings = UserSettingsManager::GetSettings();

	if (userSettings.sceneSettings.defaultOpenScene != Volt::Asset::Null())
	{
		OpenScene(userSettings.sceneSettings.defaultOpenScene);
		if (m_runtimeScene)
		{
			auto& worldEngine = m_runtimeScene->GetWorldEngineMutable();
			for (const auto& cell : worldEngine.GetCells())
			{
				worldEngine.BeginStreamingCell(cell.cellId);
			}
		}
	}

	RegisterPanels();

	m_fileWatcher = CreateRef<FileWatcher>();
	CreateWatches();

	ImGuizmo::AllowAxisFlip(false);

	InitializeModals();

	DirtySaveCustomization entityDescSaveCustomization;
	entityDescSaveCustomization.CanSaveAsset = [](const Volt::AssetHandle& handle, std::string& outCantReason) -> bool
	{
		return true;
	};
	entityDescSaveCustomization.CanUserAssignPath = [](const Volt::AssetHandle& handle)
	{
		return false;
	};
	entityDescSaveCustomization.CanSaveAssetPostCreateStep = [](const Volt::AssetHandle& asset, std::filesystem::path& outAssetNewPath, std::string& outCantReason)
	{
		Volt::ReadOnlyAssetMetadata entityMetadata = g_assetManager->GetReadOnlyAssetMetadata(asset);

		const Volt::EntityDescCustomMetadata& customMetadata = entityMetadata->GetCustomData<Volt::EntityDescCustomMetadata>();
		const Volt::AssetHandle& owningSceneHandle = customMetadata.sceneHandle;

		Volt::ReadOnlyAssetMetadata owningSceneMetadata = g_assetManager->GetReadOnlyAssetMetadata(owningSceneHandle);
		if (!owningSceneMetadata->HasFilepath())
		{
			outCantReason = "Owning Scene Does not have an associated file. Please create the scene in order to save this entity.";
			return false;
		}

		//make the path just the filename we want the entity to have, the entity desc serializer will handle the rest of the path
		outAssetNewPath = Volt::EntityDescSerialization::GetSavePathForEntity(asset);

		return true;
	};
	entityDescSaveCustomization.ShouldDeleteInstead = [](const Volt::AssetHandle& asset) -> bool
	{
		Volt::ReadOnlyAssetMetadata entityMetadata = g_assetManager->GetReadOnlyAssetMetadata(asset);

		const Volt::EntityDescCustomMetadata& customMetadata = entityMetadata->GetCustomData<Volt::EntityDescCustomMetadata>();
		const Volt::AssetHandle& owningSceneHandle = customMetadata.sceneHandle;

		VT_ENSURE(g_assetManager->IsAssetLoaded(owningSceneHandle));
		AssetReference<Volt::Scene> scene = g_assetManager->GetAssetImmediately<Volt::Scene>(owningSceneHandle);

		if (!scene->IsEntityValid(customMetadata.entityID))
		{
			return true;
		}

		return false;
	};

	DirtyAssetsManager::Get().RegisterSaveCustomizationForType(AssetTypes::EntityDesc, entityDescSaveCustomization);

	m_isInitialized = true;
}

void Sandbox::CreateWatches()
{
	m_fileWatcher->AddWatch(Volt::ProjectManager::GetEngineAssetsDirectory());
	m_fileWatcher->AddWatch(Volt::ProjectManager::GetAssetsDirectory());

	CreateModifiedWatch();
	CreateDeleteWatch();
	CreateAddWatch();
	CreateMovedWatch();
}

void Sandbox::RegisterPanels()
{
	VT_PROFILE_FUNCTION();
	// Shelved Panels (So panel tab doesn't get cluttered up).
#ifdef VT_DEBUG
	EditorLibrary::Register<ThemesPanel>("Advanced");
#endif

	EditorLibrary::Register<DebugToolsPanel>("Debug");

	EditorLibrary::Register<LogPanel>("Advanced");
	EditorLibrary::Register<RendererSettingsPanel>("Advanced", m_sceneRenderer);
	EditorLibrary::Register<RenderGraphDebuggerPanel>("Advanced", m_sceneRenderer);
	EditorLibrary::Register<EngineStatisticsPanel>("Advanced", m_runtimeScene, m_sceneRenderer, m_gameSceneRenderer);
	EditorLibrary::RegisterWithType<TextureViewerPanel>("Advanced", AssetTypes::Texture);

	EditorLibrary::Register<ProjectConversionPanel>("Advanced");

	EditorLibrary::RegisterWithType<SkeletonEditorPanel>("Animation", AssetTypes::Skeleton);
	EditorLibrary::RegisterWithType<AnimationEditorPanel>("Animation", AssetTypes::Animation);
	EditorLibrary::RegisterWithType<AnimationGraphEditorPanel>("Animation", AssetTypes::AnimationGraph);

	m_assetBrowserPanel = EditorLibrary::Register<AssetBrowserPanel>("Asset Browser", m_runtimeScene, "##Main");

	EditorLibrary::Register<PropertiesPanel>("Level Editor", m_runtimeScene, m_sceneRenderer, m_sceneState, "");
	EditorLibrary::Register<SceneViewPanel>("Level Editor", m_runtimeScene, "");
	m_viewportPanel = EditorLibrary::Register<ViewportPanel>("Level Editor", m_sceneRenderer, m_runtimeScene, m_editorCameraController.get(), m_sceneState);
	m_gameViewPanel = EditorLibrary::Register<GameViewPanel>("Level Editor", m_gameSceneRenderer, m_runtimeScene, m_sceneState);

	EditorLibrary::Register<EditorSettingsPanel>("", UserSettingsManager::GetSettings());
	EditorLibrary::Register<PhysicsPanel>("Physics");

	EditorLibrary::Register<SceneSettingsPanel>("", m_runtimeScene);
	EditorLibrary::Register<WorldEnginePanel>("", m_runtimeScene);
	EditorLibrary::Register<RenderResourcesPanel>("");
	EditorLibrary::Register<GameUIEditorPanel>("UI");

	EditorLibrary::RegisterWithType<MosaicEditorPanel>("", AssetTypes::Material);

	EditorLibrary::Sort();

	UserSettingsManager::SetupPanels();
}

void Sandbox::SetEditorHasMouseControl()
{
	Volt::Input::ShowCursor(true);
	UI::SetInputEnabled(true);
	Volt::Input::DisableInput(true);

	m_playHasMouseControl = false;
}

void Sandbox::SetPlayHasMouseControl()
{
	Volt::Input::ShowCursor(false);
	UI::SetInputEnabled(false);
	Volt::Input::DisableInput(false);

	m_playHasMouseControl = true;
}

void Sandbox::SetupNewSceneData()
{
	EditorCommandStack::Clear();

	// Scene Renderers
	{
		Volt::SceneRendererCreateInfo spec{};
		Volt::SceneRendererCreateInfo gameSpec{};

		spec.debugName = "Editor Viewport";
		spec.renderScene = m_runtimeScene->GetRenderScene();
		spec.drawDebug = true;

		gameSpec.debugName = "Game Viewport";
		gameSpec.renderScene = m_runtimeScene->GetRenderScene();

		if (m_sceneRenderer)
		{
			spec.initialResolution = { m_sceneRenderer->GetFinalImage()->GetWidth(), m_sceneRenderer->GetFinalImage()->GetHeight() };
		}

		if (m_gameSceneRenderer)
		{
			gameSpec.initialResolution = { m_gameSceneRenderer->GetFinalImage()->GetWidth(), m_gameSceneRenderer->GetFinalImage()->GetHeight() };
		}

		m_sceneRenderer = CreateRef<Volt::SceneRenderer>(spec);
		auto gridExt = m_sceneRenderer->AddExtension<GridSceneRendererExtension>(Volt::SceneRendererExtensionStage::PostPostProcessing);
		gridExt->GetIsEnabledDelegate().BindLambda([]() 
		{
			return UserSettingsManager::GetSettings().sceneSettings.gridEnabled;
		});

		m_outlineSceneRendererExtension = m_sceneRenderer->AddExtension<OutlineSceneRendererExtension>(Volt::SceneRendererExtensionStage::PostPostProcessing);
		m_objectIDSceneRendererExtension = m_sceneRenderer->AddExtension<ObjectIDSceneRendererExtension>(Volt::SceneRendererExtensionStage::PreGBuffer);

		m_gameSceneRenderer = CreateRef<Volt::SceneRenderer>(gameSpec);
	}

	Volt::OnSceneLoadedEvent loadEvent{ m_runtimeScene };
	Volt::EventSystem::DispatchEvent(loadEvent);
}

void Sandbox::InitializeModals()
{
	auto& meshModal = ModalSystem::AddModal<MeshImportModal>("Import Mesh##sandbox");
	m_meshImportModal = meshModal.GetID();

	auto& textureModal = ModalSystem::AddModal<TextureImportModal>("Import Texture##sandbox");
	m_textureImportModal = textureModal.GetID();
}

void Sandbox::OnDetach()
{
	m_isInitialized = false;

	if (m_sceneState == SceneState::Play)
	{
		OnSceneStop();
	}

	UserSettingsManager::SaveUserSettings();
	EditorLibrary::Clear();
	EditorResources::Shutdown();

	NavMeshDebugDrawer::Shutdown();

	m_fileWatcher = nullptr;
	m_editorCameraController = nullptr;
	m_sceneRenderer = nullptr;
	m_gameSceneRenderer = nullptr;

	m_gameViewPanel = nullptr;

	m_runtimeScene = nullptr;
	m_intermediateScene = nullptr;

	s_instance = nullptr;

	NodeEditorHelpers::Shutdown();
	VersionControl::Shutdown();
	SelectionManager::Shutdown();

	g_editorAssetManager = nullptr;
}

void Sandbox::OnScenePlay()
{
	m_sceneState = SceneState::Play;
	SelectionManager::DeselectAll();

	m_intermediateScene = m_runtimeScene;

	m_runtimeScene = g_assetManager->CreateMemoryAsset<Volt::Scene>("PlayInEditorScene");
	m_intermediateScene->CopyEntitiesTo(m_runtimeScene);

	SetupNewSceneData();

	SetPlayHasMouseControl();
	Volt::Input::DisableInput(false);
	m_gameViewPanel->Focus();

	m_runtimeScene->OnRuntimeStart();

	Volt::OnScenePlayEvent playEvent{};
	Volt::EventSystem::DispatchEvent(playEvent);

	Volt::ViewportResizeEvent e2 = { Volt::WindowManager::Get().GetMainWindow(), m_viewportPosition.x,m_viewportPosition.y, m_viewportSize.x, m_viewportSize.y };
	Volt::EventSystem::DispatchEvent(e2);
}

void Sandbox::OnSceneStop()
{
	SelectionManager::DeselectAll();

	Volt::OnSceneStopEvent stopEvent{};
	Volt::EventSystem::DispatchEvent(stopEvent);

	Volt::ViewportResizeEvent e2 = { Volt::WindowManager::Get().GetMainWindow(), m_viewportPosition.x,m_viewportPosition.y, m_viewportSize.x, m_viewportSize.y };
	Volt::EventSystem::DispatchEvent(e2);

	m_runtimeScene->OnRuntimeEnd();

	SetEditorHasMouseControl();
	m_viewportPanel->Focus();

	m_runtimeScene = m_intermediateScene;

	m_sceneState = SceneState::Edit;

	SetupNewSceneData();

	m_intermediateScene = nullptr;

	//Amp::MusicManager::RemoveEvent();
}

void Sandbox::OnSimulationStart()
{
	//todo: reimplement

	/*m_sceneState = SceneState::Simulating;
	SelectionManager::DeselectAll();

	m_intermediateScene = m_runtimeScene;

	m_runtimeScene = CreateRef<Volt::Scene>();
	m_intermediateScene->CopyTo(m_runtimeScene);

	SetupNewSceneData();

	m_runtimeScene->OnSimulationStart();

	Volt::OnScenePlayEvent playEvent{};
	Volt::EventSystem::DispatchEvent(playEvent);*/
}

void Sandbox::OnSimulationStop()
{
	SelectionManager::DeselectAll();

	Volt::OnSceneStopEvent stopEvent{};
	Volt::EventSystem::DispatchEvent(stopEvent);

	m_runtimeScene->OnSimulationEnd();
	m_runtimeScene = m_intermediateScene;
	m_sceneState = SceneState::Edit;

	SetupNewSceneData();
}

void Sandbox::NewScene()
{
	if (!PromptUnloadCurrentScene())
	{
		return;
	}

	SelectionManager::DeselectAll();

	m_runtimeScene = Volt::Scene::CreateDefaultScene("New Scene", true);

	SetupNewSceneData();
}

void Sandbox::OpenScene()
{
	const std::filesystem::path loadPath = FileSystem::OpenFileDialogue({ { "Scene(*.vtasset)", "vtasset" } }, Volt::ProjectManager::GetAssetsDirectory());
	OpenScene(g_assetManager->GetRelativeAssetFilepath(loadPath));
}

void Sandbox::OpenScene(const std::filesystem::path& path)
{
	if (path.empty())
	{
		return;
	}
	if (!FileSystem::Exists(Volt::ProjectManager::GetRootDirectory() / path))
	{
		UI::Notify(UI::NotificationType::Error, "Failed to Open Scene", std::format("Failed to open scene with path {}.\nFile doesnt exist!", path.string()));
		return;
	}
	const Volt::AssetHandle handle = g_assetManager->GetAssetHandleFromFilepath(path);
	Volt::ReadOnlyAssetMetadata sceneMetadata = g_assetManager->GetReadOnlyAssetMetadata(handle);

	if (sceneMetadata->type != AssetTypes::Scene)
	{
		UI::Notify(UI::NotificationType::Error, "Failed to Open Scene", std::format("Failed to open scene with path {}.\nAsset is not a Scene!", path.string()));
		return;
	}

	OpenScene(handle);
}

void Sandbox::OpenScene(Volt::AssetHandle sceneHandle)
{
	if (sceneHandle == Volt::Asset::Null())
	{
		return;
	}

	if (!g_assetManager->IsValidAssetHandle(sceneHandle))
	{
		return;
	}

	{
		Volt::ReadOnlyAssetMetadata sceneMetadata = g_assetManager->GetReadOnlyAssetMetadata(sceneHandle);
		if (sceneMetadata->type != AssetTypes::Scene)
		{
			UI::Notify(UI::NotificationType::Error, "Failed to Open Scene", std::format("Failed to open scene with handle {}.\nAsset is not a Scene!", sceneHandle));
			return;
		}
	}

	Volt::AssetHandle oldAssetHandle = Volt::Asset::Null();

	// Check if we are trying to load the same scene.
	if (m_runtimeScene)
	{
		oldAssetHandle = m_runtimeScene->GetAssetHandle();
	}

	const bool isSameScene = sceneHandle == oldAssetHandle;

	if (!isSameScene)
	{
		if (!PromptUnloadCurrentScene())
		{
			return;
		}
	}


	SelectionManager::DeselectAll();

	//load new scene
	if (!isSameScene)
	{
		AssetReference<Volt::Scene> newScene = g_assetManager->GetAssetImmediately<Volt::Scene>(sceneHandle);
		if (!newScene)
		{
			Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(sceneHandle);

			UI::Notify(UI::NotificationType::Error,
				std::format("Failed to open Scene '{0}'", assetMetadata->filepath.stem().string()),
				std::format("Failed to open scene with handle '{0}'", std::to_string(sceneHandle)));
			return;
		}

		m_runtimeScene = newScene;
	}
	else
	{
		// Reload the scene.
		g_assetManager->ReloadAsset(sceneHandle);
	}

	SetupNewSceneData();
	m_runtimeScene->LoadEntities();
}

bool Sandbox::SaveScene(bool showDialog, bool allowDiscard)
{
	//if we have no scene loaded, we successfully saved nothing!
	if (!m_runtimeScene)
	{
		return true;
	}

	SaveDirtyAssetsFilter filter;
	filter.includeAssetDelegate = [sceneHandle = m_runtimeScene->GetAssetHandle()](Volt::AssetHandle handle) -> bool
	{
		//if its the scene being unloaded, it should be included
		if (sceneHandle == handle)
		{
			return true;
		}

		//other than the owning scene we only care about entity descriptions
		Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(handle);

		if (assetMetadata->type != AssetTypes::EntityDesc)
		{
			return false;
		}

		//additionally we only care about entitites with the scene being unloaded as their owner
		if (assetMetadata->GetCustomData<Volt::EntityDescCustomMetadata>().sceneHandle != sceneHandle)
		{
			return false;
		}

		return true;
	};
	return DirtyAssetsManager::Get().SaveAssets(showDialog, allowDiscard, filter);
}

void Sandbox::InstallMayaTools()
{
	const std::filesystem::path documentsPath = FileSystem::GetDocumentsPath();
	const std::filesystem::path mayaPath = documentsPath / "maya";
	if (!std::filesystem::exists(mayaPath))
	{
		UI::Notify(UI::NotificationType::Error, "Failed to install Maya tools", "Unable to install Maya tools because no installation was found!");
		return;
	}

	for (const auto& it : std::filesystem::directory_iterator(mayaPath))
	{
		if (!it.is_directory())
		{
			continue;
		}

		const std::string folderName = it.path().stem().string();
		if (!std::all_of(folderName.begin(), folderName.end(), ::isdigit))
		{
			continue;
		}

		const std::filesystem::path pluginsPath = it.path() / "plug-ins";
		const std::filesystem::path scriptsPath = it.path() / "scripts";
		const std::filesystem::path yamlPath = scriptsPath / "yaml";

		if (!std::filesystem::exists(pluginsPath))
		{
			std::filesystem::create_directories(pluginsPath);
		}

		if (!std::filesystem::exists(scriptsPath))
		{
			std::filesystem::create_directories(scriptsPath);
		}

		if (!std::filesystem::exists(yamlPath))
		{
			std::filesystem::create_directory(yamlPath);
		}

		FileSystem::CopyFileToDirectory("../Tools/MayaExporter/voltTranslator.py", pluginsPath);
		FileSystem::CopyFileToDirectory("../Tools/MayaExporter/voltExport.py", scriptsPath);
		FileSystem::CopyFileToDirectory("../Tools/MayaExporter/voltTranslatorOpts.mel", scriptsPath);

		FileSystem::Copy("../Tools/MayaExporter/yaml", scriptsPath / "yaml");
	}

	UI::Notify(UI::NotificationType::Success, "Successfully installed Maya tools!", "The Maya tools were successfully installed!");
}

void Sandbox::RegisterEventListeners()
{
	auto isInitializedPred = [this]() { return m_isInitialized; };

	RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(Sandbox::OnUpdateEvent), isInitializedPred);
	RegisterListener<Volt::AppImGuiUpdateEvent>(VT_BIND_EVENT_FN(Sandbox::OnImGuiUpdateEvent), isInitializedPred);
	RegisterListener<Volt::AppRenderEvent>(VT_BIND_EVENT_FN(Sandbox::OnRenderEvent), isInitializedPred);
	RegisterListener<Volt::KeyPressedEvent>(VT_BIND_EVENT_FN(Sandbox::OnKeyPressedEvent), isInitializedPred);
	RegisterListener<Volt::ViewportResizeEvent>(VT_BIND_EVENT_FN(Sandbox::OnViewportResizeEvent), isInitializedPred);
	RegisterListener<Volt::OnSceneLoadedEvent>(VT_BIND_EVENT_FN(Sandbox::OnSceneLoadedEvent), isInitializedPred);

	RegisterListener<Volt::WindowTitlebarHittestEvent>([&](Volt::WindowTitlebarHittestEvent& e)
	{
		e.SetHit(m_titlebarHovered);
		return false;
	}, isInitializedPred);
}

bool Sandbox::PromptUnloadCurrentScene()
{
	//if a scene is already loaded, prompt user to save, then unload it
	if (!m_runtimeScene)
	{
		return true;
	}

	const bool userCancelSave = !SaveScene(/*showDialog*/true, true);
	if (userCancelSave)
	{
		//if the user cancels the save, dont load the new scene
		return false;
	}


	m_runtimeScene->UnloadEntities();
	m_runtimeScene.Reset();
	return true;
}

bool Sandbox::OnUpdateEvent(Volt::AppUpdateEvent& e)
{
	VT_PROFILE_FUNCTION();

	EditorCommandStack::GetInstance().Update(100);

	auto mousePos = Volt::Input::GetMousePosition();
	Volt::Input::SetViewportMousePosition(m_gameViewPanel->GetViewportLocalPosition(mousePos));

	if (m_runtimeScene)
	{
		if (m_runtimeScene->IsFinishedLoadingEntities())
		{
			switch (m_sceneState)
			{
				case SceneState::Edit:
					m_runtimeScene->UpdateEditor(e.GetTimestep());
					break;

				case SceneState::Play:
					m_runtimeScene->Update(e.GetTimestep());
					break;

				case SceneState::Pause:
					break;

				case SceneState::Simulating:
					m_runtimeScene->UpdateSimulation(e.GetTimestep());
					break;
			}
		}

		SelectionManager::Update(m_runtimeScene);
	}

	if (m_shouldResetLayout)
	{
		ImGui::LoadIniSettingsFromDisk("Editor/imgui.ini");
		m_shouldResetLayout = false;
	}

	VT_PROFILE_SCOPE("File watcher");

	std::scoped_lock lock{ m_fileWatcherMutex };
	for (const auto& f : m_fileChangeQueue)
	{
		f();
	}

	if (!m_fileChangeQueue.empty())
	{
		EditorLibrary::Get<AssetBrowserPanel>()->Reload();
	}

	m_fileChangeQueue.clear();

	return false;
}

bool Sandbox::OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e)
{
	ImGuizmo::BeginFrame();

	UpdateDockSpace();

	for (auto& window : EditorLibrary::GetPanels())
	{
		if (window.editorWindow->Begin())
		{
			window.editorWindow->UpdateMainContent();
			window.editorWindow->End();

			window.editorWindow->UpdateContent();
		}
	}

	return false;
}

void Sandbox::RenderGameView(float timestep)
{
	if (!m_gameViewPanel->IsOpen() || !m_gameSceneRenderer)
	{
		return;
	}

	switch (m_sceneState)
	{
		case SceneState::Edit:
		case SceneState::Play:
		case SceneState::Pause:
		case SceneState::Simulating:
		{
			if (!m_runtimeScene)
			{
				break;
			}

			if (!m_runtimeScene->IsFinishedLoadingEntities())
			{
				break;
			}

			Volt::Entity cameraEntity{};
			int32_t highestPrio = -1;

			m_runtimeScene->ForEachWithComponents<const Volt::CameraComponent>([&](const entt::entity id, const Volt::CameraComponent& camComp)
			{
				if ((int32_t)camComp.priority > highestPrio)
				{
					highestPrio = (int32_t)camComp.priority;
					cameraEntity = { id, m_runtimeScene->GetEntityScene() };
				}
			});

			if (!cameraEntity)
			{
				break;
			}

			const auto& camComp = cameraEntity.GetComponent<Volt::CameraComponent>();
			const auto finalImage = m_gameSceneRenderer->GetFinalImage();

			Ref<Volt::Camera> camera = CreateRef<Volt::Camera>(glm::radians(camComp.fieldOfView), (float)finalImage->GetWidth() / (float)finalImage->GetHeight(), camComp.nearPlane, camComp.farPlane);
			camera->SetPosition(cameraEntity.GetPosition());
			camera->SetRotation(glm::eulerAngles(cameraEntity.GetRotation()));

			m_gameSceneRenderer->OnRenderEditor(camera, timestep);
			break;
		}
	}
}

bool Sandbox::OnRenderEvent(Volt::AppRenderEvent& e)
{
	VT_PROFILE_FUNCTION();

	switch (m_sceneState)
	{
		case SceneState::Edit:
		case SceneState::Play:
		case SceneState::Pause:
		case SceneState::Simulating:
			if (m_sceneRenderer)
			{
				m_sceneRenderer->OnRenderEditor(m_editorCameraController->GetCamera(), e.GetTimestep());
			}
			break;
	}

	RenderGameView(e.GetTimestep());

	return false;
}

bool Sandbox::OnKeyPressedEvent(Volt::KeyPressedEvent& e)
{
	const bool ctrlPressed = Volt::Input::IsKeyDown(Volt::InputCode::LeftControl);
	const bool shiftPressed = Volt::Input::IsKeyDown(Volt::InputCode::LeftShift);

	switch (e.GetKeyCode())
	{
		case Volt::InputCode::Z:
		{
			if (ctrlPressed && shiftPressed)
			{
				EditorCommandStack::GetInstance().Redo();
			}
			else if (ctrlPressed)
			{
				EditorCommandStack::GetInstance().Undo();
			}
			break;
		}

		case Volt::InputCode::Y:
		{
			if (ctrlPressed)
			{
				EditorCommandStack::GetInstance().Redo();
			}
			break;
		}

		case Volt::InputCode::S:
		{
			if (ctrlPressed && !shiftPressed)
			{
				SaveScene();
			}
			else if (ctrlPressed && shiftPressed)
			{
				SaveScene(/*show dialog*/true);
			}

			break;
		}

		case Volt::InputCode::O:
		{
			if (ctrlPressed)
			{
				OpenScene();
			}

			break;
		}

		case Volt::InputCode::N:
		{
			if (ctrlPressed)
			{
				NewScene();
			}

			break;
		}

		case Volt::InputCode::F:
		{
			if (SelectionManager::IsAnySelected())
			{
				glm::vec3 avgPos = 0.f;

				for (const auto& id : SelectionManager::GetSelectedEntities())
				{
					Volt::Entity ent = m_runtimeScene->GetEntityFromID(id);
					avgPos += ent.GetPosition();
				}

				avgPos /= (float)SelectionManager::GetSelectedCount();

				m_editorCameraController->Focus(avgPos);
			}

			break;
		}

		case Volt::InputCode::Spacebar:
		{
			if (ctrlPressed)
			{
				for (const auto& window : EditorLibrary::GetPanels())
				{
					if (window.editorWindow->GetTitle() == "Asset Browser##Main")
					{
						if (!window.editorWindow->IsOpen())
						{
							window.editorWindow->Open();
						}
						else
						{
							window.editorWindow->Close();
						}
					}
				}
			}

			break;
		}

		case Volt::InputCode::Esc:
		{
			if (m_sceneState == SceneState::Play)
			{
				SetEditorHasMouseControl();
			}

			break;
		}

		case Volt::InputCode::G:
		{
			if (ctrlPressed && m_sceneState != SceneState::Play)
			{
				// #TODO_Ivar: Reimplement moving player to editor camera.
				OnScenePlay();
			}

			break;
		}

		default:
			break;
	}

	return false;
}

bool Sandbox::OnViewportResizeEvent(Volt::ViewportResizeEvent& e)
{
	m_runtimeScene->SetRenderSize(e.GetWidth(), e.GetHeight());
	m_viewportSize = { e.GetWidth(), e.GetHeight() };
	m_viewportPosition = { e.GetX(), e.GetY() };

	return false;
}

bool Sandbox::OnSceneLoadedEvent(Volt::OnSceneLoadedEvent& e)
{
	m_sceneRenderer->Resize(m_viewportSize.x, m_viewportSize.y);

	if (m_gameSceneRenderer)
	{
		m_gameSceneRenderer->Resize(m_viewportSize.x, m_viewportSize.y);
	}

	AssetReference<Volt::Scene> scene = e.GetScene();
	scene->SetRenderSize(m_viewportSize.x, m_viewportSize.y);

	Volt::ViewportResizeEvent e2 = { Volt::WindowManager::Get().GetMainWindow(), m_viewportPosition.x,m_viewportPosition.y, m_viewportSize.x, m_viewportSize.y };
	Volt::EventSystem::DispatchEvent(e2);

	return false;
}
