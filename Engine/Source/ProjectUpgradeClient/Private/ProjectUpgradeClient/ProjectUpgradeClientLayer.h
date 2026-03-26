#pragma once

#include <Volt-Application/ApplicationLayer.h>

#include <Volt-Core/Version.h>
#include <Volt-Core/Project/Project.h>

#include <EventSystem/EventListener.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Pointers/Ref.h>


namespace Volt
{
	class Upgrade;

	enum class UpgradeStage : uint8_t
	{
		NotStarted,
		Upgrading,
		Finished
	};

	class AppUpdateEvent;
	class AppImGuiUpdateEvent;
	class ProjectUpgradeClientLayer : public Volt::ApplicationLayer, public Volt::EventListener
	{
	public:
		ProjectUpgradeClientLayer() = default;
		~ProjectUpgradeClientLayer() override = default;

		void OnAttach() override;
		void OnDetach() override;


	private:
		bool OnUpdateEvent(Volt::AppUpdateEvent& e);
		bool OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e);

	private:
		void DrawNotStartedImGui();

		struct UpgradingProgressData
		{
			Volt::Version UpgradingToVersion;
			String ProcessingTask;

			int32_t NumTotalActions;
			int32_t NumActionsFinished;


		}m_upgradingProgressData;
		void DrawUpgradingImGui();

		void DrawFinishedImGui();

	private:
		void CollectAndOrganizeUpgrades();
		void DeserializeProject();
		void StartUpgrade();

		void UpdateProjectVersion(Volt::Version newVersion);

		Vector<Version> m_availableUpgradeVersions;

		UpgradeStage m_upgradeStage;
		Project m_targetProject;
		Version m_currentUpgradeTargetVersion;
		Ref<Upgrade> m_currentUpgrade;

		bool m_isLegacyProject = false;
	};
}
