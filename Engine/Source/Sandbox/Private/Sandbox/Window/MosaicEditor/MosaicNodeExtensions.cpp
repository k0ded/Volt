#include "sbpch.h"

#include "Sandbox/Window/MosaicEditor/MosaicNodeExtensions.h"

#include <Volt/Utility/UIUtility.h>

#include <Volt-MaterialGraph/Nodes/ConstantNodes.h>
#include <Volt-MaterialGraph/Nodes/Texture/SampleTextureNode.h>

#include <AssetSystem/AssetManager.h>

void ColorNodeExtension::Render(Ref<Mosaic::MosaicNode> node)
{
	if (node->GetGUID() == Volt::MosaicNodes::Color3Node::GetStaticGUID())
	{
		ImGui::ColorEdit3("##colorEdit3", glm::value_ptr(node->GetOutputParameter(0).Get<glm::vec3>()));
	}
	else if (node->GetGUID() == Volt::MosaicNodes::Color4Node::GetStaticGUID())
	{
		ImGui::ColorEdit4("##colorEdit4", glm::value_ptr(node->GetOutputParameter(0).Get<glm::vec4>()));
	}
}

void SampleTextureNodeExtension::Render(Ref<Mosaic::MosaicNode> node)
{
	std::string assetFileName = "Null";

	Ref<Volt::MosaicNodes::SampleTextureNode> sampleTextureNode = std::reinterpret_pointer_cast<Volt::MosaicNodes::SampleTextureNode>(node);

	Volt::AssetHandle textureHandle = sampleTextureNode->GetTextureHandle();

	const Ref<Volt::Asset> rawAsset = Volt::AssetManager::Get().GetAssetRaw(textureHandle);
	if (rawAsset)
	{
		assetFileName = rawAsset->assetName;
	}

	const ImVec2 width = ImGui::CalcTextSize(assetFileName.c_str());
	ImGui::PushItemWidth(std::max(width.x, 20.f) + 5.f);

	const std::string id = "##" + std::to_string(UI::GetID());
	ImGui::InputTextString(id.c_str(), &assetFileName, ImGuiInputTextFlags_ReadOnly);
	ImGui::PopItemWidth();

	if (auto ptr = UI::DragDropTarget("ASSET_BROWSER_ITEM"))
	{
		Volt::AssetHandle newHandle = *(Volt::AssetHandle*)ptr;
		sampleTextureNode->SetTextureHandle(newHandle);
	}
}
