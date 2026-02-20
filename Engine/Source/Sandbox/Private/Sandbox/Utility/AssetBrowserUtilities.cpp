#include "sbpch.h"

#include "Sandbox/Utility/AssetBrowserUtilities.h"
#include "Sandbox/Window/AssetBrowser/AssetItem.h"
#include "Sandbox/Window/AssetBrowser/AssetBrowserSelectionManager.h"
#include "Sandbox/UISystems/ModalSystem.h"
#include "Sandbox/Modals/MeshImportModal.h"
#include "Sandbox/Modals/TextureImportModal.h"
#include "Sandbox/Sandbox.h"

#include <Volt-Scene/Prefab.h>
#include <Volt-Scene/Scene.h>
#include <Volt-Application/UI/UIUtility.h>

#include <Volt-Assets/MeshAsset.h>

#include <Volt-CoreComponents/RenderingComponents.h>

#include <Volt-Renderer/Texture/Texture2D.h>
#include <Volt-Renderer/Utility/ImageUtility.h>

#include <AssetSystem/AssetManager.h>

#include <RHIModule/Images/Image.h>

namespace AssetBrowser
{
	const float AssetBrowserUtilities::GetBrowserItemPadding()
	{
		return 4.f;
	}

	const ImVec2 AssetBrowserUtilities::GetBrowserItemSize(const float thumbnailSize)
	{
		const float itemPadding = GetBrowserItemPadding();

		return { thumbnailSize + itemPadding, thumbnailSize + myItemHeightModifier + itemPadding };
	}

	const ImVec2 AssetBrowserUtilities::GetBrowserItemPos()
	{
		const ImVec2 cursorPos = ImGui::GetCursorPos();
		const ImVec2 windowPos = ImGui::GetWindowPos();
		const float scrollYOffset = ImGui::GetScrollY();

		return cursorPos + windowPos - ImVec2{ 0.f, scrollYOffset };
	}

	const ImVec4 AssetBrowserUtilities::GetBrowserItemHoveredColor()
	{
		return { 0.2f, 0.56f, 1.f, 1.f };
	}

	const ImVec4 AssetBrowserUtilities::GetBrowserItemClickedColor()
	{
		return { 0.3f, 0.6f, 1.f, 1.f };
	}

	const ImVec4 AssetBrowserUtilities::GetBrowserItemSelectedColor()
	{
		return { 0.f, 0.44f, 1.f, 1.f };
	}

	const ImVec4 AssetBrowserUtilities::GetBrowserItemDefaultColor()
	{
		return { 0.28f, 0.28f, 0.28f, 1.f };
	}

	const ImVec4 AssetBrowserUtilities::GetBackgroundColor(bool isHovered, bool isSelected)
	{
		ImVec4 bgColor = GetBrowserItemDefaultColor();

		if (isHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			bgColor = GetBrowserItemClickedColor();
		}
		else if (isHovered)
		{
			bgColor = GetBrowserItemHoveredColor();
		}
		else if (isSelected)
		{
			bgColor = GetBrowserItemSelectedColor();
		}

		return bgColor;
	}

	bool AssetBrowserUtilities::RenderAssetTypePopup(AssetItem* item, SelectionManager* selectionManager)
	{
		const auto& functions = GetPopupRenderFunctions();
		if (!functions.contains(item->type))
		{
			return false;
		}

		functions.at(item->type)(item, selectionManager);
		return true;
	}

	const std::unordered_map<AssetType, std::function<void(AssetItem*, SelectionManager*)>>& AssetBrowserUtilities::GetPopupRenderFunctions()
	{
		static std::unordered_map<AssetType, std::function<void(AssetItem*, SelectionManager*)>> renderFunctions;

		if (renderFunctions.empty())
		{
			renderFunctions[AssetTypes::Mesh] = [](AssetItem* item, SelectionManager* selectionManager)
			{
			};

			renderFunctions[AssetTypes::Animation] = [](AssetItem* item, SelectionManager* selectionManager)
			{
				if (ImGui::MenuItem("Reimport"))
				{
					UI::OpenModal(std::format("Reimport Animation##assetBrowser{0}", std::to_string(item->handle)));

				}
			};

			renderFunctions[AssetTypes::Skeleton] = [](AssetItem* item, SelectionManager* selectionManager)
			{
			};

			renderFunctions[AssetTypes::Prefab] = [](AssetItem* item, SelectionManager* selectionManager)
			{
			};

			renderFunctions[AssetTypes::Texture] = [](AssetItem* item, SelectionManager* selectionManager)
			{
				if (ImGui::MenuItem("Generate Mips"))
				{
					AssetReference<Volt::Texture2D> texture;
					if (g_assetManager->TryGetAsset(item->handle, texture))
					{
						Volt::ImageUtility::GenerateMipMaps(texture->GetImage());
					}
				}
			};
		}

		return renderFunctions;
	}
}
