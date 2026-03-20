#include "sbpch.h"
#include "Window/AssetBrowser/DirectoryItem.h"

#include "Sandbox/EditorAssetManager.h"
#include "Sandbox/Window/AssetBrowser/AssetBrowserSelectionManager.h"
#include "Sandbox/Window/AssetBrowser/AssetItem.h"

#include "Sandbox/Utility/EditorResources.h"
#include "Sandbox/VersionControl/VersionControl.h"

#include <Volt-Application/UI/UIUtility.h>

#include <Volt-Renderer/Texture/Texture2D.h>

#include <Volt-Core/Project/ProjectManager.h>

#include <CoreUtilities/FileSystem.h>

namespace AssetBrowser
{
	DirectoryItem::DirectoryItem(SelectionManager* selectionManager, const std::filesystem::path& path)
		: Item(selectionManager, path)
	{
		isDirectory = true;
	}

	bool DirectoryItem::Render()
	{
		bool reload = Item::Render();

		bool temp;
		if (UI::DragDropTarget("ASSET_BROWSER_ITEM", temp))// TODO: DIRECTORY UNIQUE CODE
		{
			for (const auto& item : m_selectionManager->GetSelectedItems())
			{
				if (item->isDirectory && item != this)
				{
					const std::filesystem::path newPath = path / item->path.stem();
					g_editorAssetManager->MoveDirectoryTo(item->path, newPath);
				}
			}

			for (const auto& item : m_selectionManager->GetSelectedItems())
			{
				if (!item->isDirectory && item != this && FileSystem::Exists(Volt::ProjectManager::GetRootDirectory() / item->path))
				{
					g_editorAssetManager->MoveAssetTo(g_assetManager->GetAssetHandleFromFilepath(item->path), path);
				}
			}

			reload = true;
		}

		if (UI::DragDropTarget("ASSET_BROWSER_FOLDER", temp))// TODO: DIRECTORY UNIQUE CODE
		{
			for (const auto& item : m_selectionManager->GetSelectedItems())
			{
				if (item->isDirectory && item != this)
				{
					const std::filesystem::path newPath = path / item->path.stem();
					g_editorAssetManager->MoveDirectoryTo(item->path, newPath);
				}
			}

			for (const auto& item : m_selectionManager->GetSelectedItems())
			{
				if (!item->isDirectory && item != this && FileSystem::Exists(Volt::ProjectManager::GetRootDirectory() / item->path))
				{
					g_editorAssetManager->MoveAssetTo(g_assetManager->GetAssetHandleFromFilepath(item->path), path);
				}
			}

			reload = true;
		}

		return reload;
	}

	void DirectoryItem::PushID()
	{
		ImGui::PushID(path.string().c_str());
	}

	IntRef<Volt::RHI::Image> DirectoryItem::GetIcon() const
	{
		return EditorResources::GetEditorIcon(EditorIcon::Directory);
	}

	ImVec4 DirectoryItem::GetBackgroundColor() const
	{
		return { 0.2f, 0.2f, 0.2f, 1.f };
	}

	std::string DirectoryItem::GetTypeName() const
	{
		return "Directory";
	}

	void DirectoryItem::SetDragDropPayload()
	{
		//Data being copied
		ImGui::SetDragDropPayload("ASSET_BROWSER_FOLDER", path.wstring().c_str(), path.wstring().size() * sizeof(wchar_t), ImGuiCond_Once);
	}

	bool DirectoryItem::RenderRightClickPopup()
	{
		bool removed = false;


		if (!m_selectionManager->IsSelected(this))
		{
			m_selectionManager->DeselectAll();
			m_selectionManager->Select(this);
		}

		if (ImGui::MenuItem("Show in Explorer"))
		{
			FileSystem::ShowFileInExplorer(Volt::ProjectManager::GetRootDirectory() / path);
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
			VersionControl::Edit(path);
		}


		return removed;
	}

	bool DirectoryItem::Rename(const std::string& newName)
	{
		if (newName.empty()) { return false; }

		g_editorAssetManager->RenameDirectory(path, newName);

		return true;
	}

	void DirectoryItem::Open()
	{
		isNext = true;
	}
}
