#include "sbpch.h"
#include "UserSettingsManager.h"

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Utility/EditorLibrary.h"

#include <WindowModule/WindowManager.h>

#include <Volt-FileSystem/FileUtility.h>

#include <CoreUtilities/JSON/CommonJSONSerialization.h>
#include <CoreUtilities/JSON/JSONWriter.h>
#include <CoreUtilities/JSON/JSONReader.h>

inline static const std::filesystem::path s_userSettingsPath = "User/UserSettings.json";

void UserSettingsManager::LoadUserSettings()
{
	std::string jsonString;
	if (!FileUtility::ReadStringFromFile(s_userSettingsPath, jsonString))
	{
		return;
	}

	JSONReader jsonReader;
	if (!jsonReader.Parse(jsonString))
	{
		return;
	}

	if (jsonReader.TryGet("peakNits", s_editorSettings.peakNits))
	{
		Volt::WindowManager::Get().SetPeakNits(s_editorSettings.peakNits);
	}

	jsonReader.IterateArray("Windows", [&]() 
	{
		std::string panelTitle;
		bool isOpen;

		bool success = jsonReader.TryGet("title", panelTitle);
		success |= jsonReader.TryGet("isOpen", isOpen);

		if (success && !panelTitle.empty())
		{
			s_editorSettings.panelStates.emplace_back(panelTitle, isOpen);
		}
	});

	{
		jsonReader.TryGet("sceneSettings.worldSpace", s_editorSettings.sceneSettings.worldSpace);
		jsonReader.TryGet("sceneSettings.snapToGrid", s_editorSettings.sceneSettings.snapToGrid);
		jsonReader.TryGet("sceneSettings.snapRotation", s_editorSettings.sceneSettings.snapRotation);
		jsonReader.TryGet("sceneSettings.snapScale", s_editorSettings.sceneSettings.snapScale);
		jsonReader.TryGet("sceneSettings.showGizmos", s_editorSettings.sceneSettings.showGizmos);
		jsonReader.TryGet("sceneSettings.use16by9", s_editorSettings.sceneSettings.use16by9);
		jsonReader.TryGet("sceneSettings.fullscreenOnPlay", s_editorSettings.sceneSettings.fullscreenOnPlay);
		jsonReader.TryGet("sceneSettings.gridEnabled", s_editorSettings.sceneSettings.gridEnabled);
		jsonReader.TryGet("sceneSettings.gridSnapValue", s_editorSettings.sceneSettings.gridSnapValue);
		jsonReader.TryGet("sceneSettings.rotationSnapValue", s_editorSettings.sceneSettings.rotationSnapValue);
		jsonReader.TryGet("sceneSettings.scaleSnapValue", s_editorSettings.sceneSettings.scaleSnapValue);
		jsonReader.TryGet("sceneSettings.defaultOpenScene", s_editorSettings.sceneSettings.defaultOpenScene);
		jsonReader.TryGet("sceneSettings.showLightSpheres", s_editorSettings.sceneSettings.showLightSpheres);
		jsonReader.TryGet("sceneSettings.showEntityGizmos", s_editorSettings.sceneSettings.showEntityGizmos);
		jsonReader.TryGet("sceneSettings.showBoundingSpheres", s_editorSettings.sceneSettings.showBoundingSpheres);
		jsonReader.TryGet("sceneSettings.colliderViewMode", s_editorSettings.sceneSettings.colliderViewMode);
		jsonReader.TryGet("sceneSettings.showEnvironmentProbes", s_editorSettings.sceneSettings.showEnvironmentProbes);
		jsonReader.TryGet("sceneSettings.navMeshViewMode", s_editorSettings.sceneSettings.navMeshViewMode);
	}

	{
		jsonReader.TryGet("versionControlSettings.server", s_editorSettings.versionControlSettings.server);
		jsonReader.TryGet("versionControlSettings.user", s_editorSettings.versionControlSettings.user);
		jsonReader.TryGet("versionControlSettings.password", s_editorSettings.versionControlSettings.password);
		jsonReader.TryGet("versionControlSettings.workspace", s_editorSettings.versionControlSettings.workspace);
		jsonReader.TryGet("versionControlSettings.stream", s_editorSettings.versionControlSettings.stream);
	}

	{
		jsonReader.TryGet("networkSettings.enableNetworking", s_editorSettings.networkSettings.enableNetworking);
	}

	{
		jsonReader.TryGet("networkSettings.thumbnailSize", s_editorSettings.assetBrowserSettings.thumbnailSize);
	}
}

void UserSettingsManager::SaveUserSettings()
{
	JSONWriter jsonWriter;
	jsonWriter.BeginDocument();

	jsonWriter.AppendKeyValue("peakNits", s_editorSettings.peakNits);

	jsonWriter.BeginArray("Windows");
	for (const auto& window : EditorLibrary::GetPanels())
	{
		jsonWriter.BeginObject();
		jsonWriter.AppendKeyValue("title", window.editorWindow->GetTitle());
		jsonWriter.AppendKeyValue("isOpen", window.editorWindow->IsOpen());
		jsonWriter.EndObject();
	}
	jsonWriter.EndArray();

	{
		jsonWriter.AppendKeyValue("sceneSettings.worldSpace", s_editorSettings.sceneSettings.worldSpace);
		jsonWriter.AppendKeyValue("sceneSettings.snapToGrid", s_editorSettings.sceneSettings.snapToGrid);
		jsonWriter.AppendKeyValue("sceneSettings.snapRotation", s_editorSettings.sceneSettings.snapRotation);
		jsonWriter.AppendKeyValue("sceneSettings.snapScale", s_editorSettings.sceneSettings.snapScale);
		jsonWriter.AppendKeyValue("sceneSettings.showGizmos", s_editorSettings.sceneSettings.showGizmos);
		jsonWriter.AppendKeyValue("sceneSettings.use16by9", s_editorSettings.sceneSettings.use16by9);
		jsonWriter.AppendKeyValue("sceneSettings.fullscreenOnPlay", s_editorSettings.sceneSettings.fullscreenOnPlay);
		jsonWriter.AppendKeyValue("sceneSettings.gridEnabled", s_editorSettings.sceneSettings.gridEnabled);
		jsonWriter.AppendKeyValue("sceneSettings.gridSnapValue", s_editorSettings.sceneSettings.gridSnapValue);
		jsonWriter.AppendKeyValue("sceneSettings.rotationSnapValue", s_editorSettings.sceneSettings.rotationSnapValue);
		jsonWriter.AppendKeyValue("sceneSettings.scaleSnapValue", s_editorSettings.sceneSettings.scaleSnapValue);
		jsonWriter.AppendKeyValue("sceneSettings.defaultOpenScene", s_editorSettings.sceneSettings.defaultOpenScene);
		jsonWriter.AppendKeyValue("sceneSettings.showLightSpheres", s_editorSettings.sceneSettings.showLightSpheres);
		jsonWriter.AppendKeyValue("sceneSettings.showEntityGizmos", s_editorSettings.sceneSettings.showEntityGizmos);
		jsonWriter.AppendKeyValue("sceneSettings.showBoundingSpheres", s_editorSettings.sceneSettings.showBoundingSpheres);
		jsonWriter.AppendKeyValue("sceneSettings.colliderViewMode", s_editorSettings.sceneSettings.colliderViewMode);
		jsonWriter.AppendKeyValue("sceneSettings.showEnvironmentProbes", s_editorSettings.sceneSettings.showEnvironmentProbes);
		jsonWriter.AppendKeyValue("sceneSettings.navMeshViewMode", s_editorSettings.sceneSettings.navMeshViewMode);
	}

	{
		jsonWriter.AppendKeyValue("versionControlSettings.server", s_editorSettings.versionControlSettings.server);
		jsonWriter.AppendKeyValue("versionControlSettings.user", s_editorSettings.versionControlSettings.user);
		jsonWriter.AppendKeyValue("versionControlSettings.password", s_editorSettings.versionControlSettings.password);
		jsonWriter.AppendKeyValue("versionControlSettings.workspace", s_editorSettings.versionControlSettings.workspace);
		jsonWriter.AppendKeyValue("versionControlSettings.stream", s_editorSettings.versionControlSettings.stream);
	}

	{
		jsonWriter.AppendKeyValue("networkSettings.enableNetworking", s_editorSettings.networkSettings.enableNetworking);
	}

	{
		jsonWriter.AppendKeyValue("networkSettings.thumbnailSize", s_editorSettings.assetBrowserSettings.thumbnailSize);
	}

	jsonWriter.EndDocument();

	std::string prettyJSON = jsonWriter.GetPrettyJSON();
	FileUtility::WriteStringToFile(s_userSettingsPath, std::move(prettyJSON), true);
}

void UserSettingsManager::SetupPanels()
{
	for (const auto& state : s_editorSettings.panelStates)
	{
		auto editor = EditorLibrary::GetPanel(state.panelName);
		if (editor)
		{
			if (state.isOpen)
			{
				editor->Open();
			}
			else
			{
				editor->Close();
			}
		}
	}
}
