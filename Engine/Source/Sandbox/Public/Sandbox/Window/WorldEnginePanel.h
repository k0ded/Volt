#pragma once

#include "Sandbox/Window/EditorWindow.h"

class WorldEnginePanel : public EditorWindow
{
public:
	WorldEnginePanel(AssetReference<Volt::Scene>& editorScene);

	void UpdateMainContent() override;

private:
	AssetReference<Volt::Scene>& m_editorScene;
};
