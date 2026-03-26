#include "sbpch.h"

#include "Sandbox/Window/TextureViewerPanel.h"

#include <Volt-Renderer/Texture/Texture2D.h>

#include <Volt-Application/UI/UIUtility.h>

TextureViewerPanel::TextureViewerPanel()
	: EditorWindow("Texture Viewer")
{
}

void TextureViewerPanel::UpdateMainContent()
{
	if (!m_viewingTexture)
	{
		return;
	}

	const uint32_t width = m_viewingTexture->GetWidth();
	const uint32_t height = m_viewingTexture->GetHeight();
	const uint32_t mipCount = m_viewingTexture->GetImage()->GetDesc().mips;

	Vector<String> mipStrings(mipCount);
	for (uint32_t i = 0; i < mipCount; i++)
	{
		mipStrings[i] = FormatString("Mip {}", i);
	}

	static int32_t currentMip = 0;
	static float zoomLevel = 20.f;

	UI::Combo("Mip", currentMip, mipStrings);

	ImGui::SameLine();

	ImGui::InputFloat("Zoom", &zoomLevel, 10.f);

	if (ImGui::BeginChild("Child"))
	{
		ImGui::Image(UI::GetTextureID(m_viewingTexture->GetImage(), currentMip), ImVec2{ std::floor(static_cast<float>(width) * zoomLevel * 0.01f), std::floor(static_cast<float>(height) * zoomLevel * 0.01f) });
	}
	ImGui::EndChild();
}

void TextureViewerPanel::OnClose()
{
	m_viewingTexture = nullptr;
}

void TextureViewerPanel::OpenAsset(AssetReference<Volt::Asset> asset)
{
	VT_ENSURE(asset->GetType()->GetGUID() == AssetTypes::Texture->GetGUID());
	m_viewingTexture = asset.ConvertTo<Volt::Texture2D>();
}
