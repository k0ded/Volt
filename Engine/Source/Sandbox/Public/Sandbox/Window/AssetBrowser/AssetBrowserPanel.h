#pragma once
#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/AssetBrowser/AssetCommon.h"
#include "Sandbox/Window/AssetBrowser/AssetBrowserConstants.h"

#include "Sandbox/Utility/EditorUtilities.h"


#include <CoreUtilities/Allocators/PagedArenaAllocator.h>

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
	AssetBrowserPanel(AssetReference<Volt::Scene>& aScene, const std::string& id);

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

	void Search(const std::string& query);
	void FindFoldersAndFilesWithQuery(const Vector<RawPtr<AssetBrowser::DirectoryItem>>& dirList, Vector<RawPtr<AssetBrowser::DirectoryItem>>& directories, Vector<RawPtr<AssetBrowser::AssetItem>>& assets, const std::string& query);

	AssetBrowser::DirectoryItem* FindDirectoryWithPath(const std::filesystem::path& path);
	AssetBrowser::DirectoryItem* FindDirectoryWithPathRecursivly(const Vector<RawPtr<AssetBrowser::DirectoryItem>> dirList, const std::filesystem::path& path);

	void CreatePrefabAndSetupEntities(Volt::EntityID entity);
	void SetupEntityAsPrefab(Volt::EntityID entity, Volt::AssetHandle prefabId);

	void RecursiveRemoveFolderContents(DirectoryData* aDir);
	void RecursiceRenameFolderContents(DirectoryData* aDir, const std::filesystem::path& newDir);

	void ClearAssetPreviewsInCurrentDirectory();

	float GetThumbnailSize();

	///// Asset Creation /////	
	void CreateNewAssetInCurrentDirectory(AssetType type);

	struct NewShaderData
	{
		std::string name = "New Shader";
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

	std::string mySearchQuery;
	Vector<RawPtr<AssetBrowser::DirectoryItem>> mySearchDirectories;
	Vector<RawPtr<AssetBrowser::AssetItem>> mySearchAssets;

	///// Mesh import data //////
	AssetData myMeshToImport;
	std::set<AssetType> m_assetMask;
	
	Vector<std::filesystem::path> myDragDroppedMeshes;
	Vector<std::filesystem::path> myDragDroppedTextures;

	bool myIsImporting = false;

	Volt::AssetHandle myAnimationReimportTargetSkeleton;

	AssetBrowser::DirectoryItemAllocator m_directoryItemPool;
	AssetBrowser::AssetItemAllocator m_assetItemPool;

	std::unordered_map <std::filesystem::path, RawPtr<AssetBrowser::DirectoryItem>> myDirectories;
	Ref<AssetBrowser::SelectionManager> mySelectionManager;

	AssetBrowser::DirectoryItem* myCurrentDirectory = nullptr;
	AssetBrowser::DirectoryItem* myNextDirectory = nullptr;
};
