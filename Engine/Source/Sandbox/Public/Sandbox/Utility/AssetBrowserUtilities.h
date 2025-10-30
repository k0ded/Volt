#pragma once

#include <AssetSystem/Asset.h>

#include <CoreUtilities/Containers/Vector.h>

#include <imgui.h>
#include <functional>

namespace Volt
{
	class MeshAsset;
}

namespace AssetBrowser
{
	class AssetItem;
	class SelectionManager;

	class AssetBrowserUtilities
	{
	public:
		static const float GetBrowserItemPadding();

		static const ImVec2 GetBrowserItemSize(const float thumbnailSize);
		static const ImVec2 GetBrowserItemPos();
		static float GetItemHeightModifier() { return myItemHeightModifier; };

		static const ImVec4 GetBrowserItemHoveredColor();
		static const ImVec4 GetBrowserItemClickedColor();
		static const ImVec4 GetBrowserItemSelectedColor();
		static const ImVec4 GetBrowserItemDefaultColor();

		static const ImVec4 GetBackgroundColor(bool isHovered, bool isSeleted);

		static bool RenderAssetTypePopup(AssetItem* item, SelectionManager* selectionManager);
		
	private:
		AssetBrowserUtilities() = delete;

		static constexpr float myItemHeightModifier = 90.f;

		static const std::unordered_map<AssetType, std::function<void(AssetItem*, SelectionManager*)>>& GetPopupRenderFunctions();
	};
}
