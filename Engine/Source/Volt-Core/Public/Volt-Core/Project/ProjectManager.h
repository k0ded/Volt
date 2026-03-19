#pragma once

#include "Volt-Core/Project/Project.h"
#include "Volt-Core/Config.h"

#include <LogModule/LogCategory.h>
#include <SubSystem/SubSystem.h>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Pointers/Unique.h>

VT_DECLARE_LOG_CATEGORY_EXPORT(VTCORE_API, LogProject, LogVerbosity::Trace);

namespace Volt
{
	class PluginRegistry;
	class PluginSystem;

	class VTCORE_API ProjectManager : public SubSystem
	{
	public:
		ProjectManager();
		~ProjectManager();

		ProjectManager(const ProjectManager& other) = delete;
		ProjectManager& operator=(const ProjectManager& other) = delete;

		void Initialize() override;
		void OnPostStageInitializaton() override;

		void LoadProject(const std::filesystem::path projectPath);
		void SerializeProject();

		static const std::filesystem::path GetAssetsDirectory();
		static const std::string_view GetAssetsDirectoryName();
		static const std::filesystem::path GetAudioBanksDirectory();
		static const std::filesystem::path GetProjectDirectory();
		static const std::filesystem::path GetEngineRootDirectory();
		static const std::filesystem::path GetEngineAssetsDirectory();
		static const std::filesystem::path GetPathRelativeToEngine(const std::filesystem::path& path);
		static const std::filesystem::path GetPathRelativeToProject(const std::filesystem::path& path);
		static const std::filesystem::path GetCachePath();
		static const std::filesystem::path GetOrCreateSettingsDirectory();
		static const std::filesystem::path GetPhysicsSettingsPath();
		static const std::filesystem::path GetPhysicsLayersPath();
		static const std::filesystem::path& GetRootDirectory();

		static const bool IsCurrentProjectDeprecated();
		static const bool AreCurrentProjectMetaFilesDeprecated();

		static const Project& GetProject();
		static void OnProjectUpgraded();

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{A1ED2D3C-2994-4B81-984D-23E25B9193F6}"_guid)

	private:
		inline static ProjectManager* s_instance = nullptr;

		void DeserializeProject();

		std::filesystem::path m_currentEngineDirectory;
		Unique<Project> m_currentProject;

		PluginRegistry* m_pluginRegistry = nullptr;
		PluginSystem* m_pluginSystem = nullptr;
	};
}
