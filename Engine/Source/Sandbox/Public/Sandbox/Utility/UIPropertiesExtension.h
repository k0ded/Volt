#pragma once

#include <Volt-Application/UI/UIUtility.h>
#include <concepts> // Required for std::derived_from

#include <AssetSystem/AssetManager.h>

#include <Volt-Assets/MeshAsset.h>

#include <imgui.h>

namespace Volt
{
	class Asset;
	class Scene;
	class EntityID;
}

namespace UI
{
	bool PropertyEntity(const std::string& text, Volt::Scene& scene, Volt::EntityID& value, const std::string& toolTip = "");
	bool PropertyEntity(Volt::Scene& scene, Volt::EntityID& value, const float width, const std::string& toolTip = "");

	template<typename T, typename = std::enable_if_t<std::is_base_of<Volt::Asset, T>::value>>
	bool Property(const std::string& text, AssetReference<T>& asset, const std::string& toolTip = "")
	{
		bool changed = false;

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();

		ImGui::PushItemWidth(ImGui::GetColumnWidth() - 20.f);

		std::string assetFileName = "Null";

		if (asset)
		{
			assetFileName = asset->GetAssetName();
		}

		std::string textId = MakePropertyID();
		ImGui::InputTextString(textId.c_str(), &assetFileName, ImGuiInputTextFlags_ReadOnly);
		ImGui::PopItemWidth();

		if (auto ptr = UI::DragDropTarget("ASSET_BROWSER_ITEM"))
		{
			Volt::AssetHandle newHandle = *(Volt::AssetHandle*)ptr;
			asset = g_assetManager->TryGetAssetImmediately<T>(newHandle);

			changed = true;
		}

		ImGui::SameLine();

		std::string buttonId = "X" + MakePropertyID();
		if (ImGui::Button(buttonId.c_str()))
		{
			asset = nullptr;
			changed = true;
		}

		return changed;
	}
}
