#pragma once

#include "CoreModule/Project/Project.h"
#include "CoreModule/Config.h"

#include <LogModule/LogCategory.h>
#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/Pointers/Unique.h>

VT_DECLARE_LOG_CATEGORY_EXPORT(VTC_API, LogProject, LogVerbosity::Trace);

namespace Volt
{
	class PluginRegistry;
	class PluginSystem;

	class ProjectManager : public SubSystem
	{
	public:
		VTC_API ProjectManager();
		VTC_API ~ProjectManager();

		ProjectManager(const ProjectManager& other) = delete;
		ProjectManager& operator=(const ProjectManager& other) = delete;

		void Initialize() override;
		void OnPostStageInitializaton() override;

		VTC_API void LoadProject(const Filesystem::Path& projectPath);
		VTC_API void SerializeProject();

		VTC_API static const Filesystem::Path GetAssetsDirectory();
		VTC_API static const StringView GetAssetsDirectoryName();
		VTC_API static const Filesystem::Path GetAudioBanksDirectory();
		VTC_API static const Filesystem::Path GetProjectDirectory();
		VTC_API static const Filesystem::Path GetEngineRootDirectory();
		VTC_API static const Filesystem::Path GetEngineAssetsDirectory();
		VTC_API static const Filesystem::Path GetPathRelativeToEngine(const Filesystem::Path& path);
		VTC_API static const Filesystem::Path GetPathRelativeToProject(const Filesystem::Path& path);
		VTC_API static const Filesystem::Path GetCachePath();
		VTC_API static const Filesystem::Path GetGeneratedDirectory();
		VTC_API static const Filesystem::Path GetOrCreateSettingsDirectory();
		VTC_API static const Filesystem::Path GetPhysicsSettingsPath();
		VTC_API static const Filesystem::Path GetPhysicsLayersPath();
		VTC_API static const Filesystem::Path& GetRootDirectory();

		VTC_API static const bool IsCurrentProjectDeprecated();
		VTC_API static const bool AreCurrentProjectMetaFilesDeprecated();

		VTC_API static const Project& GetProject();
		VTC_API static void OnProjectUpgraded();

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{A1ED2D3C-2994-4B81-984D-23E25B9193F6}"_guid)

	private:
		inline static ProjectManager* s_instance = nullptr;

		void DeserializeProject();

		Filesystem::Path m_currentEngineDirectory;
		Unique<Project> m_currentProject;

		PluginRegistry* m_pluginRegistry = nullptr;
		PluginSystem* m_pluginSystem = nullptr;
	};
}
