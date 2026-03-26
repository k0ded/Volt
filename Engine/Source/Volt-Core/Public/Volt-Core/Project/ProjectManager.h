#pragma once

#include "Volt-Core/Project/Project.h"
#include "Volt-Core/Config.h"

#include <LogModule/LogCategory.h>
#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

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

		void LoadProject(const Filesystem::Path& projectPath);
		void SerializeProject();

		static const Filesystem::Path GetAssetsDirectory();
		static const StringView GetAssetsDirectoryName();
		static const Filesystem::Path GetAudioBanksDirectory();
		static const Filesystem::Path GetProjectDirectory();
		static const Filesystem::Path GetEngineRootDirectory();
		static const Filesystem::Path GetEngineAssetsDirectory();
		static const Filesystem::Path GetPathRelativeToEngine(const Filesystem::Path& path);
		static const Filesystem::Path GetPathRelativeToProject(const Filesystem::Path& path);
		static const Filesystem::Path GetCachePath();
		static const Filesystem::Path GetOrCreateSettingsDirectory();
		static const Filesystem::Path GetPhysicsSettingsPath();
		static const Filesystem::Path GetPhysicsLayersPath();
		static const Filesystem::Path& GetRootDirectory();

		static const bool IsCurrentProjectDeprecated();
		static const bool AreCurrentProjectMetaFilesDeprecated();

		static const Project& GetProject();
		static void OnProjectUpgraded();

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
