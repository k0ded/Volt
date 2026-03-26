#pragma once
#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/AssetBrowser/AssetCommon.h"
#include "Sandbox/Window/AssetBrowser/AssetBrowserConstants.h"

#include "Sandbox/Utility/EditorUtilities.h"


#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

#include <glm/glm.hpp>

namespace Volt
{
	class Texture2D;
	class Scene;

	class WindowDragDropEvent;
	class KeyPressedEvent;
	class MouseButtonReleasedEvent;
	class AppRenderEvent;
}

namespace AssetBrowser
{
	class DirectoryItem;
	class AssetItem;
	class SelectionManager;
}

class AssetPreview;
class AssetBrowserPanel : public EditorWindow
{
public:
	AssetBrowserPanel(AssetReference<Volt::Scene>& aScene, const String& id);

	void UpdateMainContent() override;
	void Reload();

private:
	bool OnDragDropEvent(Volt::WindowDragDropEvent& e);
	bool OnKeyPressedEvent(Volt::KeyPressedEvent& e);
	bool OnMouseReleasedEvent(Volt::MouseButtonReleasedEvent& e);
	bool OnRenderEvent(Volt::AppRenderEvent& e);

	Vector<AssetBrowser::DirectoryItem*> FindParentDirectoriesOfDirectory(AssetBrowser::DirectoryItem* directory);

	void RenderControlsBar(float height);
	bool RenderDirectory(const RawPtr<AssetBrowser::DirectoryItem> dirData);
	void RenderView(Vector<RawPtr<AssetBrowser::DirectoryItem>>& directories, Vector<RawPtr<AssetBrowser::AssetItem>>& assets);
	void RenderWindowRightClickPopup();

	void DeleteFilesModal();

	void Search(const String& query);
	void FindFoldersAndFilesWithQuery(const Vector<RawPtr<AssetBrowser::DirectoryItem>>& dirList, Vector<RawPtr<AssetBrowser::DirectoryItem>>& directories, Vector<RawPtr<AssetBrowser::AssetItem>>& assets, const String& query);

	AssetBrowser::DirectoryItem* FindDirectoryWithPath(const Filesystem::Path& path);
	AssetBrowser::DirectoryItem* FindDirectoryWithPathRecursivly(const Vector<RawPtr<AssetBrowser::DirectoryItem>> dirList, const Filesystem::Path& path);

	void CreatePrefabAndSetupEntities(Volt::EntityID entity);
	void SetupEntityAsPrefab(Volt::EntityID entity, Volt::AssetHandle prefabId);

	void RecursiveRemoveFolderContents(DirectoryData* aDir);
	void RecursiceRenameFolderContents(DirectoryData* aDir, const Filesystem::Path& newDir);

	void ClearAssetPreviewsInCurrentDirectory();

	float GetThumbnailSize();

	///// Asset Creation /////	
	void CreateNewAssetInCurrentDirectory(AssetType type);

	struct NewShaderData
	{
		String name = "New Shader";
		bool createPixelShader = true;
		bool createGeometryShader = false;
		bool createVertexShader = false;

		int32_t shaderType = 0;

	} myNewShaderData;

	//////////////////////////

	AssetReference<Volt::Scene>& myEditorScene;

	Vector<AssetBrowser::DirectoryItem*> myDirectoryButtons;

	AssetBrowser::DirectoryItem* myAssetsDirectory = nullptr;

	float myThumbnailPadding = 16.f;
	bool myHasSearchQuery = false;
	bool myShouldDeleteSelected = false;

	glm::vec2 myViewBounds[2];

	String mySearchQuery;
	Vector<RawPtr<AssetBrowser::DirectoryItem>> mySearchDirectories;
	Vector<RawPtr<AssetBrowser::AssetItem>> mySearchAssets;

	///// Mesh import data //////
	AssetData myMeshToImport;
	std::set<AssetType> m_assetMask;
	
	Vector<Filesystem::Path> myDragDroppedMeshes;
	Vector<Filesystem::Path> myDragDroppedTextures;

	bool myIsImporting = false;
	std::atomic_bool m_reloadingAssetManager = false;
	bool m_reloadQueued = false;
	bool m_doingMainUpdate = false;

	Volt::AssetHandle myAnimationReimportTargetSkeleton;

	AssetBrowser::DirectoryItemAllocator m_directoryItemPool;
	AssetBrowser::AssetItemAllocator m_assetItemPool;

	std::unordered_map <Filesystem::Path, RawPtr<AssetBrowser::DirectoryItem>> myDirectories;
	Ref<AssetBrowser::SelectionManager> mySelectionManager;

	AssetBrowser::DirectoryItem* myCurrentDirectory = nullptr;
	AssetBrowser::DirectoryItem* myNextDirectory = nullptr;
};
