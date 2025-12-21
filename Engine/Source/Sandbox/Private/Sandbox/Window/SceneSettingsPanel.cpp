#include "sbpch.h"
#include "Window/SceneSettingsPanel.h"

#include "Sandbox/UISystems/ModalSystem.h"

#include <Volt-Application/UI/UIUtility.h>

SceneSettingsPanel::SceneSettingsPanel(AssetReference<Volt::Scene>& editorScene)
	: EditorWindow("Scene Settings"), m_editorScene(editorScene)
{
}

void SceneSettingsPanel::UpdateMainContent()
{
	auto& sceneSettings = m_editorScene->GetSceneSettingsMutable();

	if (UI::BeginProperties("sceneSettings"))
	{
		if (UI::Property("Use World Engine", sceneSettings.useWorldEngine) && sceneSettings.useWorldEngine)
		{
		}

		UI::EndProperties();
	}
}
