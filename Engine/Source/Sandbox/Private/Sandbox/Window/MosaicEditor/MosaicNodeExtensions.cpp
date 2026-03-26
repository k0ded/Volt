#include "sbpch.h"

#include "Sandbox/Window/MosaicEditor/MosaicNodeExtensions.h"

#include <Volt-Application/UI/UIUtility.h>

#include <Volt-MaterialGraph/Nodes/ConstantNodes.h>
#include <Volt-MaterialGraph/Nodes/Texture/SampleTextureNode.h>

#include <AssetSystem/AssetManager.h>

void ColorNodeExtension::Render(Ref<Mosaic::MosaicNode> node)
{
	if (node->GetGUID() == Volt::MosaicNodes::Color3::GetStaticGUID())
	{
		ImGui::ColorEdit3("##colorEdit3", glm::value_ptr(node->GetOutputParameter(0).Get<glm::vec3>()));
	}
	else if (node->GetGUID() == Volt::MosaicNodes::Color4::GetStaticGUID())
	{
		ImGui::ColorEdit4("##colorEdit4", glm::value_ptr(node->GetOutputParameter(0).Get<glm::vec4>()));
	}
}

void SampleTextureNodeExtension::Render(Ref<Mosaic::MosaicNode> node)
{
	String assetFileName = "Null";

	Ref<Volt::MosaicNodes::SampleTextureNode> sampleTextureNode = ReinterpretRefCast<Volt::MosaicNodes::SampleTextureNode>(node);

	Volt::AssetHandle textureHandle = sampleTextureNode->GetTextureHandle();

	AssetReference<Volt::Asset> rawAsset;
	if (g_assetManager->TryGetTypelessAssetIfLoaded(textureHandle, rawAsset))
	{
		assetFileName = rawAsset->GetAssetName();
	}

	const ImVec2 width = ImGui::CalcTextSize(assetFileName.c_str());
	ImGui::PushItemWidth(std::max(width.x, 20.f) + 5.f);

	const String id = FormatString("##", UI::GetAndIncrementStackID());
	ImGui::InputText(id.c_str(), &assetFileName, ImGuiInputTextFlags_ReadOnly);
	ImGui::PopItemWidth();

	Volt::AssetHandle newHandle;
	if (UI::DragDropTarget("ASSET_BROWSER_ITEM", newHandle))
	{
		sampleTextureNode->SetTextureHandle(newHandle);
	}

	bool isNormalType = sampleTextureNode->GetTextureType() == Volt::MosaicNodes::TextureType::Normal;
	if (ImGui::Checkbox("Normal Texture", &isNormalType))
	{
		if (isNormalType)
		{
			sampleTextureNode->SetTextureType(Volt::MosaicNodes::TextureType::Normal);
		}
		else
		{
			sampleTextureNode->SetTextureType(Volt::MosaicNodes::TextureType::Color);
		}
	}
}
