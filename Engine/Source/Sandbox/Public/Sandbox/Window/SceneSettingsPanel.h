#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include "Sandbox/Modals/Modal.h"

namespace Volt
{
	class Scene;
}

class SceneSettingsPanel : public EditorWindow
{
public:
	SceneSettingsPanel(AssetReference<Volt::Scene>& editorScene);

	void UpdateMainContent() override;

private:
	AssetReference<Volt::Scene>& m_editorScene;
};
