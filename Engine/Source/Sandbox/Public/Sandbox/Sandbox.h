#pragma once

#include "Sandbox/FileWatcher/FileWatcher.h"
#include "Sandbox/UISystems/ModalSystem.h"
#include "Sandbox/ComponentVisualizers/EditorDrawInterface.h"

#include <Volt-Application/ApplicationLayer.h>
#include <Volt-Scene/Scene.h>
#include <Volt-Renderer/Debug/DebugRenderer.h>

#include <AssetSystem/AssetReference.h>
#include <EventSystem/EventListener.h>
#include <EntitySystem/Entity.h>

#include <imgui.h>

#include <mutex>

namespace Volt
{
	class SceneRenderer;
	class Mesh;
	class Camera;
	class Texture2D;
	class EntityDesc;

	class Event;
	class AppUpdateEvent;
	class AppImGuiUpdateEvent;
	class AppRenderEvent;
	class KeyPressedEvent;
	class ViewportResizeEvent;
	class OnSceneLoadedEvent;
	class OnSceneTransitionEvent;
	class AssetFileCreatedEvent;
}

enum class SceneState
{
	Edit,
	Play,
	Pause,
	Simulating
};

struct ImGuiWindow;
class ViewportPanel;
class GameViewPanel;
class AssetBrowserPanel;

class EditorWindow;
class EditorCameraController;

class OutlineSceneRendererExtension;
class ObjectIDSceneRendererExtension;
class DebugSceneRendererExtension;

class Sandbox : public Volt::ApplicationLayer, public Volt::EventListener
{
public:
	Sandbox();
	~Sandbox() override;

	void OnAttach() override;
	void OnDetach() override;

	void OnScenePlay();
	void OnSceneStop();

	void OnSimulationStart();
	void OnSimulationStop();

	void SetEditorHasMouseControl();
	void SetPlayHasMouseControl();

	VT_NODISCARD VT_INLINE static Sandbox& Get() { return *s_instance; }

	Ref<Volt::SceneRenderer>& GetSceneRenderer() { return m_sceneRenderer; }
	VT_NODISCARD VT_INLINE const SceneState GetSceneState() const { return m_sceneState; }
	VT_NODISCARD VT_INLINE AssetReference<Volt::Scene> GetRuntimeScene() const { return m_runtimeScene; }
	
	VT_NODISCARD VT_INLINE UUID64 GetMeshImportModalID() const { return m_meshImportModal; }
	VT_NODISCARD VT_INLINE UUID64 GetTextureImportModalID() const { return m_textureImportModal; }
	VT_NODISCARD VT_INLINE UUID64 GetFontImportModalID() const { return m_fontImportModal; }

	VT_NODISCARD VT_INLINE Ref<ObjectIDSceneRendererExtension> GetObjectIDSceneRendererExtension() const { return m_objectIDSceneRendererExtension; }
	VT_NODISCARD VT_INLINE Ref<DebugSceneRendererExtension> GetDebugSceneRendererExtension() const { return m_debugSceneRendererExtension; }
	VT_NODISCARD VT_INLINE const Map<Volt::EntityID, VisProxyContextManager>& GetVisProxyContextManagers() const { return m_visProxyContextManagers; }

	void NewScene();
	void OpenScene();
	void OpenScene(const std::filesystem::path& path);
	void OpenScene(Volt::AssetHandle sceneHandle);
	//returns false if user cancels save
	bool SaveScene(bool showDialog = false, bool allowDiscard = false);

private:
	struct SaveSceneAsData
	{
		std::string name = "New Scene";
		std::filesystem::path destinationPath = "Assets/Scenes/";
	} m_saveSceneData;

	struct DirtyAssetExternalSaveData
	{
		bool SceneSavedAs = false;
	} m_dirtyAssetExternalSaveData;

	void InstallMayaTools();
	void RegisterEventListeners();
	//return whether to procced
	//false when user cancels unload
	bool PromptUnloadCurrentScene();

	bool OnUpdateEvent(Volt::AppUpdateEvent& e);
	bool OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e);
	bool OnRenderEvent(Volt::AppRenderEvent& e);
	bool OnKeyPressedEvent(Volt::KeyPressedEvent& e);
	bool OnViewportResizeEvent(Volt::ViewportResizeEvent& e);
	bool OnSceneLoadedEvent(Volt::OnSceneLoadedEvent& e);

	void CreateWatches();
	void RegisterPanels();

	void SetupNewSceneData();
	void InitializeModals();

	///// ImGui /////
	void UpdateDockSpace();

	void RenderWindowOuterBorders(ImGuiWindow* window);
	void HandleManualWindowResize();
	bool UpdateWindowManualResize(ImGuiWindow* window, ImVec2& newSize, ImVec2& newPosition);

	float DrawTitlebar();
	void DrawMenuBar();

	void DrawUnsavedAssetsBlock();
	void DrawDirtyAssetsExternalActionModal();
	
	void RenderGameView(float timestep);
	///////////////

	///// File Watchers /////
	void CreateModifiedWatch();
	void CreateDeleteWatch();
	void CreateAddWatch();
	void CreateMovedWatch();
	/////////////////////////

	///// Debug Rendering /////
	void DrawDebug();
	void DrawEntityGizmos();

	Map<Volt::EntityID, VisProxyContextManager> m_visProxyContextManagers;
	std::mutex m_visProxyContextManagersMutex;
	Volt::DebugRenderer m_debugRenderer;
	///////////////////////////

	Ref<EditorCameraController> m_editorCameraController;

	Ref<Volt::SceneRenderer> m_sceneRenderer;
	Ref<Volt::SceneRenderer> m_gameSceneRenderer;

	Ref<OutlineSceneRendererExtension> m_outlineSceneRendererExtension;
	Ref<ObjectIDSceneRendererExtension> m_objectIDSceneRendererExtension;
	Ref<DebugSceneRendererExtension> m_debugSceneRendererExtension;

	///// File watcher /////
	Ref<FileWatcher> m_fileWatcher;
	std::mutex m_fileWatcherMutex;
	Vector<std::function<void()>> m_fileChangeQueue;
	////////////////////////

	///// Modals /////
	UUID64 m_meshImportModal;
	UUID64 m_textureImportModal;
	UUID64 m_fontImportModal;
	//////////////////

	AssetReference<Volt::Scene> m_runtimeScene;
	AssetReference<Volt::Scene> m_intermediateScene;

	SceneState m_sceneState = SceneState::Edit;

	Ref<ViewportPanel> m_viewportPanel;
	Ref<GameViewPanel> m_gameViewPanel;

	Ref<AssetBrowserPanel> m_assetBrowserPanel;

	glm::uvec2 m_viewportSize = { 1280, 720 };
	glm::uvec2 m_viewportPosition = { 0, 0 };

	bool m_shouldOpenSaveSceneAs = false;
	bool m_openShouldSaveScenePopup = false;
	bool m_shouldResetLayout = false;
	bool m_titlebarHovered = false;
	bool m_playHasMouseControl = false;
	bool m_isInitialized = false;
	bool m_wantsToOpenCheckoutFilesModal = false;

	bool m_shouldLoadNewScene = false;
	uint32_t m_assetBrowserCount = 0;

	inline static Sandbox* s_instance = nullptr;
};
