#pragma once

#include "Sandbox/Window/EditorWindow.h"

namespace Volt
{
	class Texture2D;
}

class TextureViewerPanel : public EditorWindow
{
public:
	TextureViewerPanel();

	void UpdateMainContent() override;

	void OnClose() override;
	void OpenAsset(Ref<Volt::Asset> asset) override;

private:
	Ref<Volt::Texture2D> m_viewingTexture;
};
