#include "sbpch.h"
#include "Window/AssetBrowser/AssetBrowserPanel.h"

#include "Sandbox/Utility/EditorResources.h"

#include "Sandbox/Utility/EditorUtilities.h"
#include "Sandbox/Utility/GlobalEditorStates.h"
#include "Sandbox/Utility/Theme.h"
#include "Sandbox/Sandbox.h"

#include "Sandbox/Window/AssetBrowser/AssetItem.h"
#include "Sandbox/Window/AssetBrowser/DirectoryItem.h"
#include "Sandbox/Window/AssetBrowser/AssetBrowserSelectionManager.h"
#include "Sandbox/Window/AssetBrowser/AssetDirectoryProcessor.h"
#include "Sandbox/Modals/MeshImportModal.h"
#include "Sandbox/Modals/TextureImportModal.h"
#include "Sandbox/UserSettingsManager.h"
#include "Sandbox/DirtyAssetsManager.h"
#include "Sandbox/EditorAssetManager.h"

#include <Volt-Scene/Prefab.h>

#include <Volt-Assets/MaterialAsset.h>

#include <Volt-Animation/BlendSpace.h>

#include <Volt-Scene/Components/CoreComponents.h>
#include <Volt-Scene/Scene.h>
#include <Volt-Application/UI/UIUtility.h>

#include <Volt-Physics/PhysicsMaterialAsset.h>

#include <Volt-Core/Project/ProjectManager.h>

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/FileIO/YAMLFileStreamWriter.h>
#include <CoreUtilities/FileSystem.h>

#include <EventSystem/Event.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>
#include <WindowModule/Events/WindowEvents.h>

#include <InputModule/Input.h>
#include <InputModule/InputCodes.h>
#include <InputModule/Events/KeyboardEvents.h>
#include <InputModule/Events/MouseEvents.h>

#include <JobSystem/JobSystem.h>

#undef CreateDirectory

AssetBrowserPanel::AssetBrowserPanel(AssetReference<Volt::Scene>& aScene, const std::string& id)
	: EditorWindow("Asset Browser" + id), myEditorScene(aScene)
{
	m_windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	m_backgroundColor = EditorTheme::MiddleGreyBackground;
	Open();

	RegisterListener<Volt::WindowDragDropEvent>(VT_BIND_EVENT_FN(AssetBrowserPanel::OnDragDropEvent));
	RegisterListener<Volt::KeyPressedEvent>(VT_BIND_EVENT_FN(AssetBrowserPanel::OnKeyPressedEvent));
	RegisterListener<Volt::AppRenderEvent>(VT_BIND_EVENT_FN(AssetBrowserPanel::OnRenderEvent));

	SetMinWindowSize({ 700.f, 300.f });

	mySelectionManager = CreateRef<AssetBrowser::SelectionManager>();


	Reload();
}

void AssetBrowserPanel::UpdateMainContent()
{
	//we need to reload again if we request a reload during another reload
	if (m_reloadQueued && !m_reloadingAssetManager)
	{
		m_reloadQueued = false;
		Reload();
		return;
	}

	if (m_reloadingAssetManager)
	{
		ImGui::Text("Discovering Assets...");
		return;
	}

	if (myDirectories.empty())
	{
		ImGui::Text("No directory... Try refreshing :)");
		return;
	}

	m_doingMainUpdate = true;

	float cellSize = GetThumbnailSize() + myThumbnailPadding;

	if (myNextDirectory)
	{
		mySelectionManager->DeselectAll();
		ClearAssetPreviewsInCurrentDirectory();
		myCurrentDirectory = myNextDirectory;
		myNextDirectory = nullptr;

		myDirectoryButtons.clear();
		myDirectoryButtons = FindParentDirectoriesOfDirectory(myCurrentDirectory);
	}

	const float controlsBarHeight = 30.f;

	// Controls bar
	{
		UI::ScopedStyleFloat rounding(ImGuiStyleVar_FrameRounding, 2.f);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
		RenderControlsBar(controlsBarHeight);
		ImGui::PopStyleVar();
	}

	const ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
	if (ImGui::BeginTable("assetBrowserMain", 2, tableFlags))
	{
		ImGui::TableSetupColumn("Outline", 0, 250.f);
		ImGui::TableSetupColumn("View", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		bool reload = false;

		//Draw outline
		{
			ImGuiStyle& style = ImGui::GetStyle();
			auto color = style.Colors[ImGuiCol_FrameBg];

			UI::ScopedColor newColor(ImGuiCol_ChildBg, { color.x, color.y, color.z, color.w });
			UI::ScopedStyleFloat rounding(ImGuiStyleVar_ChildRounding, 2.f);

			if (ImGui::BeginChild("##outline"))
			{
				UI::ScopedColor headerColor{ ImGuiCol_Header, { 0.f } };
				UI::ScopedColor headerColorActive{ ImGuiCol_HeaderActive, { 0.f } };
				UI::ScopedColor headerColorHovered{ ImGuiCol_HeaderHovered, { 0.f } };

				UI::ShiftCursor(5.f, 5.f);

				const bool selected = myCurrentDirectory == myAssetsDirectory;
				const auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen | (selected ? ImGuiTreeNodeFlags_Selected : 0);

				UI::RenderHighlightedBackground(EditorTheme::ItemChildActive, 17.f);
				bool open = UI::TreeNodeImage(EditorResources::GetEditorIcon(EditorIcon::Directory), "Assets", flags);

				if (ImGui::IsItemClicked())
				{
					mySelectionManager->DeselectAll();
					myNextDirectory = myAssetsDirectory;
				}

				if (open)
				{
					UI::ScopedStyleFloat2 spacing(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

					for (const auto& subDir : myAssetsDirectory->subDirectories)
					{
						reload |= RenderDirectory(subDir);
						if (reload)
						{
							break;
						}
					}
					UI::TreeNodePop();
				}

				ImGui::EndChild();
			}
		}

		if (reload)
		{
			Reload();
			ImGui::EndTable();
			return;
		}

		ImGui::TableNextColumn();

		if (ImGui::BeginChild("##view", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetWindowHeight() - controlsBarHeight)))
		{
			const auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
			const auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			const auto viewportOffset = ImGui::GetWindowPos();

			myViewBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
			myViewBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

			ImGui::BeginChild("Scrolling");
			{
				float panelWidth = ImGui::GetContentRegionAvail().x;
				auto columnCount = (int)(panelWidth / cellSize);

				if (columnCount < 1)
				{
					columnCount = 1;
				}

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.f, 0.f, 0.f, 0.f });

				if (ImGui::BeginTable("##viewTable", columnCount))
				{
					ImGui::TableNextColumn();

					if (!myHasSearchQuery)
					{
						RenderView(myCurrentDirectory->subDirectories, myCurrentDirectory->assets);
					}
					else
					{
						RenderView(mySearchDirectories, mySearchAssets);
					}

					ImGui::EndTable();
				}

				RenderWindowRightClickPopup();

				ImGui::PopStyleColor();
			}
			ImGui::EndChild();

			Volt::EntityID entityId;
			if (UI::DragDropTarget("scene_entity_hierarchy", entityId))
			{
				if (entityId != Volt::Entity::NullID())
				{
					CreatePrefabAndSetupEntities(entityId);
					Reload();
				}
			}
			ImGui::EndChild();
		}

		ImGui::EndTable();
	}

	if (myShouldDeleteSelected)
	{
		UI::OpenModal("Delete Selected Files?");
		myShouldDeleteSelected = false;
	}
	DeleteFilesModal();

	m_doingMainUpdate = false;
}

bool AssetBrowserPanel::OnDragDropEvent(Volt::WindowDragDropEvent& e)
{
	float x = Volt::Input::GetMouseX();
	float y = Volt::Input::GetMouseY();
	const auto [wX, wY] = Volt::WindowManager::Get().GetMainWindow().GetPosition();

	x += wX;
	y += wY;

	if (x > myViewBounds[0].x && y > myViewBounds[0].y && x < myViewBounds[1].x && y < myViewBounds[1].y)
	{
		for (const auto& path : e.GetPaths())
		{
			// #TODO_Editor: Add better support for drag dropping
			if (EditorUtils::IsAssetTypeFileExtension(AssetTypes::MeshSource, path))
			{
				auto& modal = ModalSystem::GetModal<MeshImportModal>(Sandbox::Get().GetMeshImportModalID());
				modal.SetImportMeshes({ path });
				modal.SetDestinationDirectory(g_assetManager->GetAssetFilesystemPath(myCurrentDirectory->path));
				modal.Open();
			}
			else if (EditorUtils::IsAssetTypeFileExtension(AssetTypes::TextureSource, path))
			{
				auto& modal = ModalSystem::GetModal<TextureImportModal>(Sandbox::Get().GetTextureImportModalID());
				modal.SetImportTextures({ path });
				modal.SetDestinationDirectory(g_assetManager->GetAssetFilesystemPath(myCurrentDirectory->path));
				modal.Open();
			}

			break;
		}

		Reload();
	}

	return false;
}

bool AssetBrowserPanel::OnKeyPressedEvent(Volt::KeyPressedEvent& e)
{
	switch (e.GetKeyCode())
	{
		case Volt::InputCode::Delete:
		{
			if (IsFocused() && mySelectionManager->IsAnySelected())
			{
				myShouldDeleteSelected = true;
			}

			break;
		}
	}

	return false;
}

bool AssetBrowserPanel::OnMouseReleasedEvent(Volt::MouseButtonReleasedEvent& e)
{
	if (e.GetMouseButton() == Volt::InputCode::Mouse_LB)
	{
		if (ImGui::IsWindowHovered() && GlobalEditorStates::isDragging)
		{
			GlobalEditorStates::isDragging = false;
			GlobalEditorStates::dragStartedInAssetBrowser = false;
			GlobalEditorStates::dragAsset = Volt::Asset::Null();
		}
	}

	return false;
}

bool AssetBrowserPanel::OnRenderEvent(Volt::AppRenderEvent& e)
{
	if (!myCurrentDirectory)
	{
		return false;
	}

	return false;
}

Vector<AssetBrowser::DirectoryItem*> AssetBrowserPanel::FindParentDirectoriesOfDirectory(AssetBrowser::DirectoryItem* directory)
{
	Vector<AssetBrowser::DirectoryItem*> directories;
	directories.emplace_back(directory);

	for (auto dir = directory->parentDirectory; dir != nullptr; dir = dir->parentDirectory)
	{
		directories.emplace_back(dir);
	}

	std::reverse(directories.begin(), directories.end());
	return directories;
}

void AssetBrowserPanel::RenderControlsBar(float height)
{
	UI::ScopedColor childColor{ ImGuiCol_ChildBg, { 0.2f, 0.2f, 0.2f, 1.f } };

	if (ImGui::BeginChild("##controlsBar", { 0.f, std::min(height, ImGui::GetContentRegionAvail().y) }))
	{
		const float buttonSizeOffset = 10.f;
		int32_t offsetToRemove = 0;
		bool shouldRemove = false;

		UI::ShiftCursor(5.f, 4.f);
		{
			UI::ScopedColor buttonBackground(ImGuiCol_Button, { 0.f, 0.f, 0.f, 0.f });
			ImGui::Image(UI::GetTextureID(EditorResources::GetEditorIcon(EditorIcon::Search)), { height - buttonSizeOffset, height - buttonSizeOffset });

			ImGui::SameLine();
			UI::ShiftCursor(0.f, -0.5f);
			ImGui::PushItemWidth(200.f);

			if (UI::InputTextWithHint("", mySearchQuery, "Search..."))
			{
				if (!mySearchQuery.empty())
				{
					myHasSearchQuery = true;
					Search(mySearchQuery);
				}
				else
				{
					myHasSearchQuery = false;
				}
			}

			ImGui::PopItemWidth();
			ImGui::SameLine();
			UI::ShiftCursor(0.f, -1.f);
			{
				if (UI::ImageButton("##reloadButton", UI::GetTextureID(EditorResources::GetEditorIcon(EditorIcon::Reload)), { height - buttonSizeOffset, height - buttonSizeOffset }))
				{
					Reload();
				}

				ImGui::SameLine();

				if (UI::ImageButton("##backButton", UI::GetTextureID(EditorResources::GetEditorIcon(EditorIcon::Back)), { height - buttonSizeOffset, height - buttonSizeOffset }))
				{
					myHasSearchQuery = false;
					mySearchQuery.clear();

					if (myCurrentDirectory->path != Volt::ProjectManager::GetAssetsDirectory())
					{
						if (myCurrentDirectory->parentDirectory != nullptr)
						{
							myNextDirectory = myCurrentDirectory->parentDirectory;

							offsetToRemove = (uint32_t)(myDirectoryButtons.size() - 1);
							shouldRemove = true;
						}
					}
				}
			}

			for (size_t i = 0; i < myDirectoryButtons.size(); i++)
			{
				ImGui::SameLine();

				std::string dirName = myDirectoryButtons[i]->path.stem().string();

				const float buttonWidth = ImGui::CalcTextSize(dirName.c_str()).x + 5.f;
				UI::ScopedColor bgColor(ImGuiCol_Button, { 0.5f, 0.5f, 0.5f, 1.f });

				if (ImGui::BeginChild(myDirectoryButtons[i]->path.string().c_str(), { buttonWidth, height - 4.f }))
				{
					const bool hovered = ImGui::IsWindowHovered();
					if (hovered)
					{
						ImGui::PushStyleColor(ImGuiCol_Text, { 0.4f, 0.67f, 1.000f, 1.000f });
					}

					UI::ShiftCursor(0.f, 4.f);
					ImGui::TextUnformatted(dirName.c_str());

					Volt::AssetHandle handle;
					if (UI::DragDropTarget({ "ASSET_BROWSER_ITEM" }, handle))
					{
						g_editorAssetManager->MoveAssetTo(handle, myDirectoryButtons.at(i)->path);
						Reload();

						ImGui::EndChild();

						if (hovered)
						{
							ImGui::PopStyleColor();
						}

						break;
					}

					if (hovered)
					{
						ImGui::PopStyleColor();
					}

					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
					{
						myNextDirectory = myDirectoryButtons[i];

						offsetToRemove = (int32_t)(i + 1);
						shouldRemove = true;

						myHasSearchQuery = false;
						mySearchQuery.clear();
					}

					ImGui::EndChild();
				}

				ImGui::SameLine();

				if (myDirectoryButtons.size() - 1 > i)
				{
					ImGui::TextUnformatted("/");
				}
			}

			ImGui::SameLine();

			UI::ShiftCursor(ImGui::GetContentRegionAvail().x - 2.f * height - buttonSizeOffset, 0.f);

			// Filter button
			{
				UI::ImageButton("##filter", UI::GetTextureID(EditorResources::GetEditorIcon(EditorIcon::Filter)), { height - buttonSizeOffset, height - buttonSizeOffset });

				if (ImGui::BeginPopupContextItem("filterMenu", ImGuiPopupFlags_MouseButtonLeft))
				{
					UI::ScopedColor clearButtonBackground(ImGuiCol_Button, { 0.3f, 0.3f, 0.3f, 1.f });

					if (ImGui::Button("Clear##filterMenu"))
					{
						m_assetMask.clear();
						Reload();
					}

					for (const auto& [guid, type] : AssetTypeRegistry::Get().GetTypeMap())
					{
						bool selected = m_assetMask.contains(type);
						if (ImGui::Checkbox(type->GetName().data(), &selected))
						{
							if (selected)
							{
								m_assetMask.insert(type);
							}
							else
							{
								m_assetMask.erase(type);
							}

							Reload();
						}
					}
					ImGui::EndPopup();
				}
			}

			ImGui::SameLine();

			// Settings button
			{
				ImGui::ImageButton("##AssetBrowserSettingsButton",UI::GetTextureID(EditorResources::GetEditorIcon(EditorIcon::Settings)), {height - buttonSizeOffset, height - buttonSizeOffset});
				if (ImGui::BeginPopupContextItem("settingsMenu", ImGuiPopupFlags_MouseButtonLeft))
				{
					ImGui::PushItemWidth(100.f);
					ImGui::SliderFloat("Icon size", &UserSettingsManager::GetSettings().assetBrowserSettings.thumbnailSize, 20.f, 200.f);
					ImGui::PopItemWidth();

					ImGui::EndPopup();
				}
			}


			if (shouldRemove)
			{
				for (int32_t i = (int32_t)myDirectoryButtons.size() - 1; i >= offsetToRemove; i--)
				{
					myDirectoryButtons.erase(myDirectoryButtons.begin() + i);
				}
			}
		}
	}
	ImGui::EndChild();
}

bool AssetBrowserPanel::RenderDirectory(const RawPtr<AssetBrowser::DirectoryItem> dirData)
{
	bool reload = false;

	auto isAnyDecendantActive = [&](RawPtr<AssetBrowser::DirectoryItem> dirData, auto isAnyDecendantActive, bool first)
	{
		if (myCurrentDirectory == dirData.GetRaw() && !first)
		{
			return true;
		}

		for (const auto& subDir : dirData->subDirectories)
		{
			if (isAnyDecendantActive(subDir, isAnyDecendantActive, false))
			{
				return true;
			}
		}

		return false;
	};

	const bool isDecendantActive = isAnyDecendantActive(dirData, isAnyDecendantActive, true);
	const bool selected = mySelectionManager->IsSelected(dirData.GetRaw()) || myCurrentDirectory == dirData.GetRaw() || isDecendantActive;
	const auto flags = (selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None) | ImGuiTreeNodeFlags_OpenOnArrow;

	bool hovered = false;
	constexpr float itemHeight = 17.f;

	// Check if item is hovered
	{
		const auto windowPos = ImGui::GetWindowPos();
		const auto availRegion = ImGui::GetContentRegionMax();
		const auto cursorPos = ImGui::GetCursorPos();

		const ImVec2 min = ImGui::GetWindowPos() + ImVec2{ 0.f, cursorPos.y };
		const ImVec2 max = ImGui::GetWindowPos() + ImVec2{ availRegion.x, itemHeight + cursorPos.y };
		hovered = ImGui::IsMouseHoveringRect(min, max);
	}

	// Draw background
	if (hovered)
	{
		UI::RenderHighlightedBackground(EditorTheme::ItemHovered, itemHeight);
	}
	else if (isDecendantActive)
	{
		UI::RenderHighlightedBackground(EditorTheme::ItemChildActive, itemHeight);
	}
	else if (selected)
	{
		UI::RenderHighlightedBackground(EditorTheme::ItemSelected, itemHeight);
	}


	const std::string id = dirData->path.stem().string() + "##" + dirData->path.string();
	const bool open = UI::TreeNodeImage(EditorResources::GetEditorIcon(EditorIcon::Directory), id, flags, isDecendantActive);

	if (ImGui::IsItemClicked() && !selected)
	{
		mySelectionManager->Select(dirData.GetRaw());
		myNextDirectory = dirData.GetRaw();
	}

	bool temp;
	if (UI::DragDropTarget({ "ASSET_BROWSER_ITEM", "ASSET_BROWSER_FOLDER" }, temp))
	{
		for (const auto& item : mySelectionManager->GetSelectedItems())
		{
			if (item->isDirectory && item != dirData.GetRaw())
			{
				const std::filesystem::path newPath = dirData->path / item->path.stem();
				g_editorAssetManager->MoveDirectoryTo(item->path, newPath);
			}
		}

		for (const auto& item : mySelectionManager->GetSelectedItems())
		{
			if (!item->isDirectory && item != dirData.GetRaw() && std::filesystem::exists(Volt::ProjectManager::GetRootDirectory() / item->path))
			{
				g_editorAssetManager->MoveAssetTo(g_assetManager->GetAssetHandleFromFilepath(item->path), dirData->path);
			}
		}

		reload = true;
	}

	if (open)
	{
		for (const auto& subDir : dirData->subDirectories)
		{
			RenderDirectory(subDir);
		}

		for (const auto& asset : dirData->assets)
		{
			std::string assetId = asset->path.stem().string() + "##" + std::to_string(asset->handle);
			ImGui::Selectable(assetId.c_str());
		}

		ImGui::TreePop();
	}

	return reload;
}

void AssetBrowserPanel::RenderView(Vector<RawPtr<AssetBrowser::DirectoryItem>>& directories, Vector<RawPtr<AssetBrowser::AssetItem>>& assets)
{
	bool reload = false;

	bool skipEntitiesDir = false;
	for (const auto& asset : assets) { if (asset->type == AssetTypes::Scene) { skipEntitiesDir = true; break; } }

	for (const auto& dir : directories)
	{
		if (skipEntitiesDir && (dir->path.filename() == "Entities" || dir->path.filename() == "Layers")) { continue; }

		const bool changed = dir->Render();
		ImGui::TableNextColumn();

		if (changed)
		{
			reload = true;
			break;
		}

		if (dir->isNext)
		{
			myNextDirectory = dir.GetRaw();
			dir->isNext = false;
		}
	}

	if (reload)
	{
		reload = false;
		Reload();
	}

	for (const auto& asset : assets)
	{
		const bool changed = asset->Render();
		ImGui::TableNextColumn();

		if (changed)
		{
			reload = true;
			break;
		}

		if (UI::BeginModal(std::format("Reimport Animation##assetBrowser{0}", std::to_string(asset->handle))))
		{
			if (UI::BeginProperties())
			{
				EditorUtils::Property("Target Skeleton", myAnimationReimportTargetSkeleton, AssetTypes::Skeleton);

				UI::EndProperties();
			}

			if (ImGui::Button("Reimport"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}

			UI::EndModal();
		}
	}

	if (reload)
	{
		Reload();
	}

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		mySelectionManager->DeselectAll();
	}

	UI::ShiftCursor(0.f, 200.f); // Extra space at the bottom
}

void AssetBrowserPanel::RenderWindowRightClickPopup()
{
	if (ImGui::BeginPopupContextWindow("CreateMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_NoOpenOverItems))
	{
		ImGui::SetCursorPosX(150.f);
		ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x);

		if (ImGui::BeginMenu("New"))
		{
			if (ImGui::BeginMenu("Materials##Menu"))
			{
				if (ImGui::MenuItem("Material"))
				{
					CreateNewAssetInCurrentDirectory(AssetTypes::Material);
				}

				if (ImGui::MenuItem("Mosaic Graph"))
				{
					CreateNewAssetInCurrentDirectory(AssetTypes::Material);
				}

				if (ImGui::MenuItem("Post Processing Stack"))
				{
					CreateNewAssetInCurrentDirectory(AssetTypes::PostProcessingStack);
				}

				if (ImGui::MenuItem("Post Processing Material"))
				{
					CreateNewAssetInCurrentDirectory(AssetTypes::PostProcessingMaterial);
				}

				if (ImGui::MenuItem("Physics Material"))
				{
					CreateNewAssetInCurrentDirectory(AssetTypes::PhysicsMaterial);
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Animation##Menu"))
			{
				if (ImGui::MenuItem("Blend Space"))
				{
					CreateNewAssetInCurrentDirectory(AssetTypes::BlendSpace);
				}

				ImGui::EndMenu();
			}

			UI::SmallSeparatorHeader("Other", 5.f);

			if (ImGui::MenuItem("Scene"))
			{
				CreateNewAssetInCurrentDirectory(AssetTypes::Scene);
			}

			UI::SmallSeparatorHeader("File system", 5.f);

			if (ImGui::MenuItem("Folder"))
			{
				const std::string originalName = "New Folder";
				std::string tempName = originalName;

				uint32_t i = 0;
				while (FileSystem::Exists(Volt::ProjectManager::GetRootDirectory() / myCurrentDirectory->path / tempName))
				{
					tempName = originalName + " (" + std::to_string(i) + ")";
					i++;
				}

				FileSystem::CreateDirectories(Volt::ProjectManager::GetRootDirectory() / myCurrentDirectory->path / tempName);
				Reload();

				auto dirIt = std::find_if(myCurrentDirectory->subDirectories.begin(), myCurrentDirectory->subDirectories.end(), [tempName](const RawPtr<AssetBrowser::DirectoryItem> data)
				{
					return data->path.stem().string() == tempName;
				});

				if (dirIt != myCurrentDirectory->subDirectories.end())
				{
					(*dirIt)->StartRename();

					mySelectionManager->Select((*dirIt).GetRaw());
				}
			}


			ImGui::EndMenu();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Reload"))
		{
			Reload();
		}

		ImGui::EndPopup();
	}

}

void AssetBrowserPanel::DeleteFilesModal()
{
	UI::ScopedStyleFloat buttonRounding{ ImGuiStyleVar_FrameRounding, 2.f };

	if (UI::BeginModal("Delete Selected Files?", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Are you sure you want to delete the selected files?");

		ImGui::PushItemWidth(80.f);
		if (ImGui::Button("Yes"))
		{
			const auto& selectedItems = mySelectionManager->GetSelectedItems();

			for (const auto& item : selectedItems)
			{
				if (item->isDirectory)
				{
					g_editorAssetManager->DeleteDirectory(item->path);
				}
			}

			for (const auto& item : selectedItems)
			{
				Volt::AssetHandle itemAssetHandle = g_assetManager->GetAssetHandleFromFilepath(item->path);

				if (!item->isDirectory && g_assetManager->IsValidAssetHandle(itemAssetHandle))
				{
					g_editorAssetManager->DeleteAsset(itemAssetHandle);
				}
			}

			Reload();
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("No"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::PopItemWidth();

		UI::EndModal();
	}
}

void AssetBrowserPanel::Reload()
{
	if (!g_assetManager->GetMetadataLoadingCounter()->IsCompleted() ||
		m_reloadingAssetManager ||
		m_doingMainUpdate)
	{
		m_reloadQueued = true;
		return;
	}
	m_reloadingAssetManager = true;

	Volt::JobSystem::RunJob(Volt::JobSystem::CreateJob("Reload Asset Browser...", Volt::ExecutionPriority::Latent, [this]()
	{
		const std::filesystem::path currentPath = myCurrentDirectory ? myCurrentDirectory->path : Volt::ProjectManager::GetAssetsDirectory();

		Vector<AssetBrowser::DirectoryItem*> directoriesToClear;
		{
			Vector<AssetBrowser::DirectoryItem*> directoriesToTraverse;
			for (auto& [path, dir] : myDirectories)
			{
				directoriesToTraverse.emplace_back(dir.GetRaw());
			}

			while (!directoriesToTraverse.empty())
			{
				AssetBrowser::DirectoryItem* dir = directoriesToTraverse.back();
				directoriesToTraverse.pop_back();
				directoriesToClear.emplace_back(dir);
				for (RawPtr<AssetBrowser::DirectoryItem> subDir : dir->subDirectories)
				{
					directoriesToTraverse.emplace_back(subDir.GetRaw());
				}
			}
		}

		for (AssetBrowser::DirectoryItem* dir : directoriesToClear)
		{
			for (RawPtr<AssetBrowser::AssetItem> assetItem : dir->assets)
			{
				m_assetItemPool.Free(assetItem.GetRaw());
			}

			m_directoryItemPool.Free(dir);
		}

		myCurrentDirectory = nullptr;
		myNextDirectory = nullptr;
		mySelectionManager->DeselectAll();

		ClearAssetPreviewsInCurrentDirectory();

		if (!Volt::ProjectManager::GetProject().isDeprecated)
		{
			AssetDirectoryProcessor processor{ mySelectionManager, m_assetMask, m_directoryItemPool, m_assetItemPool };
			myDirectories[Volt::ProjectManager::GetAssetsDirectory()] = processor.ProcessDirectories(Volt::ProjectManager::GetAssetsDirectory(), myMeshToImport);
		}

		myAssetsDirectory = myDirectories[Volt::ProjectManager::GetAssetsDirectory()].GetRaw();

		//Find directory
		myCurrentDirectory = FindDirectoryWithPath(currentPath);
		if (!myCurrentDirectory)
		{
			myCurrentDirectory = myAssetsDirectory;
		}

		//Setup new file path buttons
		myDirectoryButtons.clear();
		myDirectoryButtons = FindParentDirectoriesOfDirectory(myCurrentDirectory);

		m_reloadingAssetManager = false;
	}));
}

void AssetBrowserPanel::Search(const std::string& inQuery)
{
	Vector<std::string> queries;
	Vector<std::string> types;

	std::string searchQuery = inQuery;
	searchQuery.push_back(' ');

	for (auto next = searchQuery.find_first_of(' '); next != std::string::npos; next = searchQuery.find_first_of(' '))
	{
		std::string split = searchQuery.substr(0, next);
		searchQuery = searchQuery.substr(next + 1);

		if (split.front() == '*')
		{
			types.emplace_back(split.substr(1));
		}
		else
		{
			queries.emplace_back(split);
		}
	}

	//Find all folders and files containing queries
	mySearchDirectories.clear();
	mySearchAssets.clear();
	for (const auto& query : queries)
	{
		FindFoldersAndFilesWithQuery(myAssetsDirectory->subDirectories, mySearchDirectories, mySearchAssets, query);
	}

	for (const auto& type : types)
	{
		FindFoldersAndFilesWithQuery(myAssetsDirectory->subDirectories, mySearchDirectories, mySearchAssets, type);
	}
}

void AssetBrowserPanel::FindFoldersAndFilesWithQuery(const Vector<RawPtr<AssetBrowser::DirectoryItem>>& dirList, Vector<RawPtr<AssetBrowser::DirectoryItem>>& directories, Vector<RawPtr<AssetBrowser::AssetItem>>& assets, const std::string& query)
{
	for (const auto& dir : dirList)
	{
		std::string dirStem = dir->path.stem().string();
		std::transform(dirStem.begin(), dirStem.end(), dirStem.begin(), [](unsigned char c)
		{
			return std::tolower(c);
		});

		if (dirStem.find(query) != std::string::npos)
		{
			directories.emplace_back(dir);
		}

		for (const auto& asset : dir->assets)
		{
			std::string assetFilename = asset->path.filename().string();
			std::transform(assetFilename.begin(), assetFilename.end(), assetFilename.begin(), [](unsigned char c)
			{
				return std::tolower(c);
			});

			if (assetFilename.find(query) != std::string::npos)
			{
				assets.emplace_back(asset);
			}
		}

		FindFoldersAndFilesWithQuery(dir->subDirectories, directories, assets, query);
	}
}

AssetBrowser::DirectoryItem* AssetBrowserPanel::FindDirectoryWithPath(const std::filesystem::path& path)
{
	Vector<RawPtr<AssetBrowser::DirectoryItem>> dirList;
	for (const auto& dir : myDirectories)
	{
		dirList.emplace_back(dir.second);
	}

	return FindDirectoryWithPathRecursivly(dirList, path);
}

AssetBrowser::DirectoryItem* AssetBrowserPanel::FindDirectoryWithPathRecursivly(const Vector<RawPtr<AssetBrowser::DirectoryItem>> dirList, const std::filesystem::path& path)
{
	for (const auto& dir : dirList)
	{
		if (dir->path == path)
		{
			return dir.GetRaw();
		}
	}

	for (const auto& dir : dirList)
	{
		if (auto it = FindDirectoryWithPathRecursivly(dir->subDirectories, path))
		{
			return it;
		}
	}

	return nullptr;
}

void AssetBrowserPanel::CreatePrefabAndSetupEntities(Volt::EntityID id)
{
	Volt::Entity entity = myEditorScene->GetEntityFromID(id);

	if (entity.HasComponent<Volt::PrefabComponent>())
	{
		UI::Notify(UI::NotificationType::Error, "Unable to create prefab!", "Cannot create prefab of existing prefab!");
		return;
	}

	const auto& tagComp = entity.GetComponent<Volt::TagComponent>();

	std::string name = tagComp.tag;
	name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end());

	AssetReference<Volt::Prefab> prefab = g_assetManager->CreateAssetAndFile<Volt::Prefab>(g_assetManager->GetRelativeAssetFilepath(myCurrentDirectory->path), name, entity);

	SetupEntityAsPrefab(entity.GetID(), prefab->GetAssetHandle());
	Reload();
}

void AssetBrowserPanel::SetupEntityAsPrefab(Volt::EntityID id, Volt::AssetHandle prefabId)
{
	Volt::Entity entity = myEditorScene->GetEntityFromID(id);

	if (!entity.HasComponent<Volt::PrefabComponent>())
	{
		entity.AddComponent<Volt::PrefabComponent>();
	}

	auto& prefabComp = entity.GetComponent<Volt::PrefabComponent>();
	prefabComp.prefabAsset = prefabId;
	prefabComp.prefabEntity = entity.GetID();

	auto& relComp = entity.GetComponent<Volt::RelationshipComponent>();
	for (const auto& child : relComp.children)
	{
		SetupEntityAsPrefab(child, prefabId);
	}
}

void AssetBrowserPanel::RecursiveRemoveFolderContents(DirectoryData* aDir)
{
	for (const auto& asset : aDir->assets)
	{
		if (FileSystem::Exists(Volt::ProjectManager::GetRootDirectory() / asset.path))
		{
			g_editorAssetManager->DeleteAsset(asset.handle);
		}
	}

	aDir->assets.clear();

	for (const auto& dir : aDir->subDirectories)
	{
		if (FileSystem::Exists(Volt::ProjectManager::GetRootDirectory() / dir->path))
		{
			RecursiveRemoveFolderContents(dir.get());
			FileSystem::Remove(Volt::ProjectManager::GetRootDirectory() / dir->path);
		}
	}

	aDir->subDirectories.clear();
}

void AssetBrowserPanel::RecursiceRenameFolderContents(DirectoryData* aDir, const std::filesystem::path& newDir)
{
	FileSystem::MoveDirectory(aDir->path, newDir);
	aDir->path = newDir / aDir->path.filename();

	for (const auto& asset : aDir->assets)
	{
		g_editorAssetManager->MoveAssetTo(asset.handle, newDir);
	}

	for (const auto& dir : aDir->subDirectories)
	{
		RecursiceRenameFolderContents(dir.get(), aDir->path);
	}
}

void AssetBrowserPanel::ClearAssetPreviewsInCurrentDirectory()
{
	//for (const auto& asset : myCurrentDirectory->assets)
	//{
	//	switch (asset->type)
	//	{
	//		case AssetType::Mesh:
	//			asset->preview = CreateRef<AssetPreview>(asset->path);
	//			myPreviewsToUpdate.emplace_back(asset->preview);
	//			break;
	//	}
	//}
}

float AssetBrowserPanel::GetThumbnailSize()
{
	return UserSettingsManager::GetSettings().assetBrowserSettings.thumbnailSize;

}

void AssetBrowserPanel::CreateNewAssetInCurrentDirectory(AssetType type)
{
	std::string originalName;
	std::string tempName;
	uint32_t i = 0;

	if (type == AssetTypes::Material) originalName = "M_NewMaterial";
	if (type == AssetTypes::PhysicsMaterial) originalName = "PM_NewPhysicsMaterial";
	if (type == AssetTypes::Scene) originalName = "SC_NewScene";
	if (type == AssetTypes::BlendSpace) originalName = "BS_NewBlendSpace";
	if (type == AssetTypes::PostProcessingStack) originalName = "PPS_NewPostStack";
	if (type == AssetTypes::PostProcessingMaterial) originalName = "PPM_NewPostMaterial";

	tempName = originalName;

	const std::string ext = ".vtasset";
	while (FileSystem::Exists(Volt::ProjectManager::GetRootDirectory() / g_assetManager->GetRelativeAssetFilepath(myCurrentDirectory->path) / (tempName + ext)))
	{
		tempName = originalName + " (" + std::to_string(i) + ")";
		i++;
	}

	if (type == AssetTypes::Material)
	{
		g_assetManager->CreateAssetAndFile<Volt::MaterialAsset>(g_assetManager->GetRelativeAssetFilepath(myCurrentDirectory->path), tempName);
	}
	else if (type == AssetTypes::Scene)
	{
		FileSystem::CreateDirectories(g_assetManager->GetRelativeAssetFilepath(myCurrentDirectory->path) / tempName);

		AssetReference<Volt::Scene> scene = Volt::Scene::CreateDefaultScene("New Scene");

		const std::filesystem::path targetFilePath = (g_assetManager->GetRelativeAssetFilepath(myCurrentDirectory->path / tempName / (tempName + ext)));
		g_assetManager->CreateFileForAsset(scene->GetAssetHandle(), targetFilePath);
	}
	else if (type == AssetTypes::BlendSpace)
	{
		g_assetManager->CreateAssetAndFile<Volt::BlendSpace>(g_assetManager->GetRelativeAssetFilepath(myCurrentDirectory->path), tempName);
	}

	Reload();

	auto assetIt = std::find_if(myCurrentDirectory->assets.begin(), myCurrentDirectory->assets.end(), [&tempName](const auto& lhs)
	{
		return lhs->path.stem().string() == tempName;
	});

	if (assetIt != myCurrentDirectory->assets.end())
	{
		(*assetIt)->StartRename();

		mySelectionManager->Select((*assetIt).GetRaw());
	}
}
