#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <AssetSystem/AssetHandle.h>

class DebugToolsPanel : public EditorWindow
{
public:
	DebugToolsPanel();
	~DebugToolsPanel() override;

	void UpdateMainContent() override;

private://scene loading
	void DrawLoadSceneMultipleTimes();
	void UpdateSceneLoading();
	struct SceneLoadingData
	{
		SceneLoadingData()
		{
			Reset();
		}
		void Reset()
		{
			numFinishedLoading = 0;
			numToLoad = 100;
			sceneToLoad = Volt::AssetHandle(0);

			running = false;
		}

		int32_t numFinishedLoading;
		int32_t numToLoad;
		Volt::AssetHandle sceneToLoad;

		bool running;
	} m_sceneLoadingData;

};
