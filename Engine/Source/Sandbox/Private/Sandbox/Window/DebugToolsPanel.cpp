#include "sbpch.h"
#include "Sandbox/Window/DebugToolsPanel.h"

#include "Sandbox/Utility/EditorUtilities.h"

#include "Sandbox/Sandbox.h"

#include <Volt-Application/UI/UIProperties.h>
#include <Volt-Scene/AssetTypes.h>
#include <Volt-Scene/Scene.h>

#include <imgui.h>

DebugToolsPanel::DebugToolsPanel()
	: EditorWindow("Debug Tools")
{}

DebugToolsPanel::~DebugToolsPanel()
{}

void DebugToolsPanel::UpdateMainContent()
{
	UpdateSceneLoading();
	if (ImGui::CollapsingHeader("Load Scene Many Times"))
	{
		ImGui::Indent();
		DrawLoadSceneMultipleTimes();
		ImGui::Unindent();
	}
}

void DebugToolsPanel::DrawLoadSceneMultipleTimes()
{
	ImGui::BeginDisabled(m_sceneLoadingData.running);

	UI::BeginProperties();
	EditorUtils::Property("Scene To Load", m_sceneLoadingData.sceneToLoad, AssetTypes::Scene);

	UI::Property("Num Times To Load", m_sceneLoadingData.numToLoad);
	UI::EndProperties();

	ImGui::EndDisabled();

	if (m_sceneLoadingData.running)
	{
		const float fraction = static_cast<float>(m_sceneLoadingData.numFinishedLoading) / static_cast<float>(m_sceneLoadingData.numToLoad);
		ImGui::ProgressBar(fraction, { 200.f,  ImGui::CalcTextSize("").y + 4.f }, std::format("{0}/{1}", m_sceneLoadingData.numFinishedLoading, m_sceneLoadingData.numToLoad).c_str());

		if (ImGui::Button("Stop"))
		{
			m_sceneLoadingData.running = false;
		}
	}
	else
	{
		if (ImGui::Button("Run"))
		{
			m_sceneLoadingData.numFinishedLoading = 0;
			m_sceneLoadingData.running = true;
		}
	}
}

void DebugToolsPanel::UpdateSceneLoading()
{
	if (!m_sceneLoadingData.running)
	{
		return;
	}

	if (m_sceneLoadingData.numFinishedLoading >= m_sceneLoadingData.numToLoad)
	{
		m_sceneLoadingData.running = false;
		return;
	}

	bool loadLevel = true;
	{
		AssetReference<Volt::Scene> scene = Sandbox::Get().GetRuntimeScene();
		if (scene)
		{
			loadLevel = scene->IsFinishedLoadingEntities();
		}
	}

	if (loadLevel)
	{
		Sandbox::Get().OpenScene(m_sceneLoadingData.sceneToLoad);
		m_sceneLoadingData.numFinishedLoading++;
	}
}
