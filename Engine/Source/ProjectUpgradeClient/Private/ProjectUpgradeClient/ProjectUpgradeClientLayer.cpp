#include "ProjectUpgradeClient/ProjectUpgradeClientLayer.h"
#include "ProjectUpgradeClient/Common/YAMLFileStreamWriter.h"
#include "ProjectUpgradeClient/Common/YAMLFileStreamReader.h"

#include "ProjectUpgradeClient/UpgradesRegistry.h"
#include "ProjectUpgradeClient/UpgradeInterface.h"

#include "ProjectUpgradeClient/LegacyUpgrades/LegacyUpgrade.h"

#include <EventSystem/ApplicationEvents.h>

#include <Volt-Application/BaseApplication.h>
#include <Volt-Application/UI/UIUtility.h>
#include <Volt-Application/UI/ImGuiSubSystem.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/FileSystem.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <imgui.h>
#include <imgui_stdlib.h>

namespace Volt
{
	void ProjectUpgradeClientLayer::OnAttach()
	{
		m_upgradeStage = UpgradeStage::NotStarted;

		RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(ProjectUpgradeClientLayer::OnUpdateEvent));
		RegisterListener<Volt::AppImGuiUpdateEvent>(VT_BIND_EVENT_FN(ProjectUpgradeClientLayer::OnImGuiUpdateEvent));

		const CommandLineBuilder& commandLineBuilder = BaseApplication::Get().GetCommandLineBuilder();
		m_isLegacyProject = commandLineBuilder.IsArgDefined("legacy");

		if (!m_isLegacyProject)
		{
			DeserializeProject();
			CollectAndOrganizeUpgrades();
		}
	}

	void ProjectUpgradeClientLayer::OnDetach()
	{}

	bool ProjectUpgradeClientLayer::OnUpdateEvent(Volt::AppUpdateEvent& e)
	{
		return false;
	}

	bool ProjectUpgradeClientLayer::OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e)
	{
		auto& io = ImGui::GetIO();

		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ io.DisplaySize.x, io.DisplaySize.y });

		if (ImGui::Begin("Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize))
		{
			switch (m_upgradeStage)
			{
				case Volt::UpgradeStage::NotStarted:
					DrawNotStartedImGui();
					break;
				case Volt::UpgradeStage::Upgrading:
					DrawUpgradingImGui();
					break;
				case Volt::UpgradeStage::Finished:
					DrawFinishedImGui();
					break;
			}
		}
		/*ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->DrawList;
		ImVec2 desiredSize = ImGui::GetCurrentWindow()->ContentRegionRect.GetSize();
		Volt::WindowManager::Get().GetMainWindow().Resize(static_cast<uint32_t>(desiredSize.x), static_cast<uint32_t>(desiredSize.y));*/
		ImGui::End();

		return false;
	}
	void ProjectUpgradeClientLayer::DrawNotStartedImGui()
	{
		ImGui::Text("Engine Version: %s", VT_VERSION.ToString().c_str());

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.823f, 0.169f, 0.169f, 1.f));
		ImGui::Text("Project Version: %s", m_targetProject.engineVersion.ToString().c_str());
		ImGui::PopStyleColor();

		ImGui::Separator();

		if (VT_VERSION < m_targetProject.engineVersion.ToString())
		{
			ImGui::Text("Your engine is an older version than the project.");

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.823f, 0.169f, 0.169f, 1.f));
			ImGui::Text("It is not possible to downgrade a project.");
			ImGui::PopStyleColor();

			ImGui::Text("Please get engine version '%s' or newer.", m_targetProject.engineVersion.ToString().c_str());

			if (ImGui::Button("OK"))
			{
				BaseApplication::Get().Quit();
			}
		}
		else
		{
			ImGui::Text("Your engine is a newer version than the project.");
			ImGui::Text("Your project needs to be upgraded to use this engine version.");

			ImGui::Separator();

			ImGui::Text("Upgrades to run:");
			ImGui::Indent();
			for (int i = static_cast<int>(m_availableUpgradeVersions.size()) - 1; i >= 0; i--)
			{
				const Volt::Version& upgradeVersion = m_availableUpgradeVersions[i];
				ImGui::Text(upgradeVersion.ToString().c_str());
			}
			ImGui::Unindent();

			ImGui::Spacing();

			if (ImGui::Button("Upgrade"))
			{
				StartUpgrade();
			}
			ImGui::SameLine();
			if (ImGui::Button("Quit"))
			{
				BaseApplication::Get().Quit();
			}
		}
	}
	void ProjectUpgradeClientLayer::DrawUpgradingImGui()
	{
		ImGui::Text("Upgrading...");

		ImGui::Separator();

		if (m_isLegacyProject)
		{
			const CommandLineBuilder& commandLineBuilder = BaseApplication::Get().GetCommandLineBuilder();

			Ref<LegacyProjectUpgrade> legacyUpgrade = CreateRef<LegacyProjectUpgrade>(m_targetProject);
			legacyUpgrade->TryConvertProject(commandLineBuilder.GetArgValue("project"), commandLineBuilder.GetArgValue("target"));

			m_currentUpgrade = legacyUpgrade;
		}
		else
		{
			if (!m_currentUpgrade)
			{
				m_currentUpgradeTargetVersion = m_availableUpgradeVersions.back();
				m_availableUpgradeVersions.pop_back();

				m_currentUpgrade = UpgradesRegistry::Get().CreateUpgrade(m_currentUpgradeTargetVersion, m_targetProject);
			}
		}

		bool doneProcessing = m_currentUpgrade->ProcessUpgrade();

		if (doneProcessing)
		{
			m_currentUpgrade.Reset();
			if (m_availableUpgradeVersions.empty())
			{
				m_upgradeStage = UpgradeStage::Finished;

				UpdateProjectVersion(VT_VERSION);

				return;
			}
		}

		float progressFraction = 1.f;
		if (m_currentUpgrade)
		{
			const size_t total = (m_currentUpgrade->GetNumTotalActions() != 0) ? m_currentUpgrade->GetNumTotalActions() : 1;
			progressFraction = static_cast<float>(m_currentUpgrade->GetNumActionsCompleted()) / static_cast<float>(total);
			ImGui::Text("%s", m_currentUpgrade->GetCurrentActionText().c_str());
			ImGui::Text("%d/%d", m_currentUpgrade->GetNumActionsCompleted(), m_currentUpgrade->GetNumTotalActions());
		}

		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		constexpr float loadingBarRounding = 3.f;
		const ImVec2 loadingBarSize = ImVec2(200, 30);

		ImVec2 loadingBarMin = ImGui::GetCursorScreenPos();
		ImVec2 loadingBarMax = loadingBarMin + loadingBarSize;
		ImVec2 loadingBarProgressMax = loadingBarMin + ImVec2(loadingBarSize.x * progressFraction, loadingBarSize.y);

		DrawList->PushClipRect(loadingBarMin, loadingBarMax);

		DrawList->AddRect(loadingBarMin, loadingBarMax, ImColor(0.3f, 0.3f, 0.3f, 1.f), loadingBarRounding, 0, 3.f);
		DrawList->AddRectFilled(loadingBarMin, loadingBarMax, ImColor(1.f, 0.f, 0.f, 1.f), loadingBarRounding);
		DrawList->AddRectFilled(loadingBarMin, loadingBarProgressMax, ImColor(0.f, 1.f, 0.f, 1.f), loadingBarRounding);
		DrawList->PopClipRect();
	}
	void ProjectUpgradeClientLayer::DrawFinishedImGui()
	{
		ImGui::Text("Finished upgrading project!");
		if (ImGui::Button("Done"))
		{
			BaseApplication::Get().Quit();
		}
	}

	void ProjectUpgradeClientLayer::CollectAndOrganizeUpgrades()
	{
		auto& upgradeRegistryMap =  UpgradesRegistry::Get().GetRegistry();

		for (auto [version, upgradeInfo] : upgradeRegistryMap)
		{
			if (version <= m_targetProject.engineVersion)
			{
				continue;
			}
			m_availableUpgradeVersions.push_back(version);
		}
		//latest version first so we can pop_back while upgrading
		std::sort(m_availableUpgradeVersions.begin(), m_availableUpgradeVersions.end(), [](const auto& lhs, const auto& rhs)
		{
			return lhs > rhs;
		});
	}

	void ProjectUpgradeClientLayer::DeserializeProject()
	{
		const CommandLineBuilder& commandLineBuilder = BaseApplication::Get().GetCommandLineBuilder();
		std::filesystem::path projectFilepath;

		VT_ASSERT_MSG(commandLineBuilder.IsArgDefined("project"), "Project Upgrade Client expects a project to be provided!");
		projectFilepath = commandLineBuilder.GetArgValue("project");


		//get the project engine version
		YAMLFileStreamReader streamReader{};

		VT_ASSERT_MSG(streamReader.OpenFile(projectFilepath), std::format("Failed to open file: {0}!", projectFilepath.string()));
		VT_ASSERT_MSG(streamReader.HasKey("Project"), std::format("Project file {0} is invalid!", projectFilepath.string()));

		streamReader.EnterScope("Project");

		m_targetProject.engineVersion = streamReader.ReadAtKey("EngineVersion", std::string(""));
		m_targetProject.name = streamReader.ReadAtKey("Name", std::string("None"));
		m_targetProject.companyName = streamReader.ReadAtKey("CompanyName", std::string("None"));
		m_targetProject.assetsDirectoryName = streamReader.ReadAtKey("AssetsDirectory", std::string("Assets"));
		m_targetProject.audioDirectory = streamReader.ReadAtKey("AudioBanksDirectory", std::filesystem::path("Audio/Banks"));
		m_targetProject.iconFilepath = streamReader.ReadAtKey("IconPath", std::filesystem::path(""));
		m_targetProject.cursorFilepath = streamReader.ReadAtKey("CursorPath", std::filesystem::path(""));
		m_targetProject.startSceneFilepath = streamReader.ReadAtKey("StartScene", std::filesystem::path(""));

		m_targetProject.rootDirectory = projectFilepath.parent_path();
		m_targetProject.filepath = projectFilepath;

		//dont need plugins in here 

		streamReader.ExitScope();
	}

	void ProjectUpgradeClientLayer::StartUpgrade()
	{
		m_upgradeStage = UpgradeStage::Upgrading;
	}
	void ProjectUpgradeClientLayer::UpdateProjectVersion(Volt::Version newVersion)
	{
		const CommandLineBuilder& commandLineBuilder = BaseApplication::Get().GetCommandLineBuilder();
		std::filesystem::path projectFilepath;

		VT_ASSERT_MSG(commandLineBuilder.IsArgDefined("project"), "Project Upgrade Client expects a project to be provided!");
		projectFilepath = commandLineBuilder.GetArgValue("project");

		Vector<std::string> pluginNames;
		{
			YAMLFileStreamReader streamReader{};

			VT_ASSERT_MSG(streamReader.OpenFile(projectFilepath), std::format("Failed to open file: {0}!", projectFilepath.string()));
			VT_ASSERT_MSG(streamReader.HasKey("Project"), std::format("Project file {0} is invalid!", projectFilepath.string()));
			streamReader.GetRawNode();

			streamReader.EnterScope("Project");

			streamReader.ForEach("Plugins", [&]()
			{
				const std::string pluginName = streamReader.ReadValue<std::string>();
				pluginNames.push_back(pluginName);
			});
			streamReader.ExitScope();
		}

		if (!FileSystem::IsWriteable(projectFilepath))
		{
			FileSystem::MakeWriteable(projectFilepath);
		}

		YAMLFileStreamWriter streamWriter{ projectFilepath };

		streamWriter.BeginMap();
		streamWriter.BeginMapNamned("Project");

		streamWriter.SetKey("EngineVersion", VT_VERSION.ToString());
		streamWriter.SetKey("Name", m_targetProject.name);
		streamWriter.SetKey("CompanyName", m_targetProject.companyName);
		streamWriter.SetKey("AssetsDirectory", m_targetProject.assetsDirectoryName);
		streamWriter.SetKey("AudioBanksDirectory", m_targetProject.audioDirectory);
		streamWriter.SetKey("IconPath", m_targetProject.iconFilepath);
		streamWriter.SetKey("CursorPath", m_targetProject.cursorFilepath);
		streamWriter.SetKey("StartScenePath", m_targetProject.startSceneFilepath);

		streamWriter.BeginSequence("Plugins");
		for (const std::string& plugin : pluginNames)
		{
			streamWriter.AddValue(plugin);
		}
		streamWriter.EndSequence();

		streamWriter.EndMap();
		streamWriter.EndMap();
		streamWriter.WriteToDisk();
	}
}
