#include "sbpch.h"
#include "Window/AssetBrowser/AssetItem.h"

#include "Sandbox/Sandbox.h"
#include "Sandbox/EditorAssetManager.h"
#include "Sandbox/Window/AssetBrowser/AssetBrowserSelectionManager.h"

#include "Sandbox/Utility/EditorResources.h"
#include "Sandbox/Utility/EditorUtilities.h"
#include "Sandbox/Utility/AssetBrowserUtilities.h"

#include "Sandbox/Utility/GlobalEditorStates.h"
#include "Sandbox/Utility/EditorLibrary.h"
#include "Sandbox/Utility/PremadeCommands.h"

#include "Sandbox/VersionControl/VersionControl.h"
#include "Sandbox/UserSettingsManager.h"
#include "Sandbox/Window/AssetBrowser/EditorAssetRegistry.h"

#include <AssetSystem/AssetManager.h>

#include <Volt-Renderer/Texture/Texture2D.h>

#include <Volt-Scene/AssetTypes.h>

#include <Volt-Application/UI/UIUtility.h>

#include <CoreUtilities/StringUtility.h>

#include <Volt-Animation/Assets/AssetTypes.h>

namespace AssetBrowser
{
	AssetItem::AssetItem(SelectionManager* selectionManager, const std::filesystem::path& path, AssetData& aMeshToImportData, Volt::AssetHandle inHandle)
		: Item(selectionManager, path), meshToImportData(aMeshToImportData), handle(inHandle)
	{
		// Assign a random handle to non registered assets
		if (handle == Volt::Asset::Null())
		{
			handle = {};
		}
		else
		{
			Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(handle);
			type = assetMetadata->type;
		}
	}

	bool AssetItem::Render()
	{
		bool reload = Item::Render();

		/*if (SaveReturnState returnState = EditorUtils::SaveFilePopup("Do you want to save scene?##OpenSceneAssetBrowser"); returnState != SaveReturnState::None)
		{
			if (returnState == SaveReturnState::Save)
			{
				Sandbox::Get().SaveScene();
			}

			Sandbox::Get().OpenScene(Volt::AssetManager::GetFilePathFromAssetHandle(mySceneToOpen));
			mySceneToOpen = Volt::Asset::Null();
		}*/

		return reload;
	}

	void AssetItem::PushID()
	{
		ImGui::PushID(static_cast<int32_t>(handle));
	}

	bool AssetItem::RenderRightClickPopup()
	{
		bool removed = false;
		if (!m_selectionManager->IsSelected(this))
		{
			m_selectionManager->DeselectAll();
			m_selectionManager->Select(this);
		}

		if (ImGui::MenuItem("Open Externally"))
		{
			FileSystem::OpenFileExternally(Volt::ProjectManager::GetRootDirectory() / path);
		}

		if (ImGui::MenuItem("Show In Explorer"))
		{
			FileSystem::ShowFileInExplorer(Volt::ProjectManager::GetRootDirectory() / path);
		}

		if (ImGui::MenuItem("Reload"))
		{
			g_assetManager->ReloadAsset(handle);
		}

		ImGui::Separator();

		bool extraItemsRendered = AssetBrowserUtilities::RenderAssetTypePopup(this, m_selectionManager);

		if (extraItemsRendered)
		{
			ImGui::Separator();
		}

		if (ImGui::MenuItem("Rename"))
		{
			StartRename();
		}

		if (ImGui::MenuItem("Delete"))
		{
			UI::OpenModal("Delete Selected Files?");
		}

		if (ImGui::MenuItem("Checkout"))
		{
			VersionControl::Edit(g_assetManager->GetAssetFilesystemPath(handle));
		}

		return removed;
	}

	bool AssetItem::Rename(const std::string& aNewName)
	{
		if (aNewName.empty()) { return false; }

		g_editorAssetManager->RenameAsset(handle, aNewName);

		return true;
	}

	void AssetItem::Open()
	{
		if (!EditorLibrary::OpenAsset(handle))
		{
			if (type == AssetTypes::Scene)
			{
				Sandbox::Get().OpenScene(handle);
			}
		}
	}

	void AssetItem::DrawAdditionalHoverInfo()
	{
		auto extraData = EditorAssetRegistry::GetAssetBrowserPopupData(type, handle);
		for (auto& data : extraData)
		{
			DrawHoverInfo(data.first, data.second);
		}
		//file size
		{
			const auto fullPath = Volt::ProjectManager::GetAssetsDirectory() / std::filesystem::relative(path, "Assets\\");
			uintmax_t fileSize = 0;
			if (std::filesystem::exists(fullPath))
			{
				fileSize = std::filesystem::file_size(fullPath);
			}

			const std::string sizeStringWithMetricPrefix = Utility::ToStringWithMetricPrefixCharacterForBytes(fileSize);
			const std::string sizeStringWithSeparator = Utility::ToStringWithThousandSeparator(fileSize);

			DrawHoverInfo("Size", sizeStringWithMetricPrefix + " (" + sizeStringWithSeparator +" bytes)");
		}
	}

	RefPtr<Volt::RHI::Image> AssetItem::GetIcon() const
	{
		RefPtr<Volt::RHI::Image> icon = previewImage ? previewImage : nullptr;
		if (!icon && EditorResources::GetAssetIcon(type))
		{
			icon = EditorResources::GetAssetIcon(type);
		}
		else
		{
			icon = EditorResources::GetEditorIcon(EditorIcon::GenericFile);
		}
		return icon;
	}

	ImVec4 AssetItem::GetBackgroundColor() const
	{
		return GetBackgroundColorFromType(type);
	}

	std::string AssetItem::GetTypeName() const
	{
		return std::string(type->GetName());
	}

	void AssetItem::SetDragDropPayload()
	{
		//Data being copied
		ImGui::SetDragDropPayload("ASSET_BROWSER_ITEM", &handle, sizeof(Volt::AssetHandle), ImGuiCond_Once);

		// Viewport Drag drop
		GlobalEditorStates::dragStartedInAssetBrowser = true;
		GlobalEditorStates::dragAsset = handle;
		GlobalEditorStates::isDragging = true;
	}

	const ImVec4 AssetItem::GetBackgroundColorFromType(AssetType assetType) const
	{
		if (assetType == AssetTypes::Mesh) return { 0.73f, 0.9f, 0.26f, 1.f };
		if (assetType == AssetTypes::MeshSource) return { 0.43f, 0.9f, 0.26f, 1.f };
		if (assetType == AssetTypes::NavMesh) return { 0.f, 0.80f, 0.98f, 1.f };
		if (assetType == AssetTypes::Animation) return { 0.65f, 0.18f, 0.69f, 1.f };
		if (assetType == AssetTypes::Skeleton) return { 1.f, 0.49f, 0.8f, 1.f };
		if (assetType == AssetTypes::Texture) return { 0.9f, 0.26f, 0.27f, 1.f };
		if (assetType == AssetTypes::Material) return { 0.26f, 0.35f, 0.9f, 1.f };
		if (assetType == AssetTypes::Scene) return { 0.9f, 0.54f, 0.26f, 1.f };
		if (assetType == AssetTypes::Prefab) return { 0.25f, 0.93f, 0.92f, 1.f };
		if (assetType == AssetTypes::BehaviorGraph) return { 0.75f, 0.04f, 0.83f, 1.f };
		if (assetType == AssetTypes::EnvironmentTexture) return { 0.5f, 0.26f, 0.8f, 1.f };

		return { 0.f, 0.f, 0.f, 1.f };
	}
}
