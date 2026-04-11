#include "cpch.h"
#include "CoreModule/Project/ProjectManager.h"
#include "CoreModule/PluginSystem/PluginSystem.h"
#include "CoreModule/PluginSystem/PluginRegistry.h"
#include "CoreModule/AppVersion.h"
#include "CoreModule/GlobalCommandLine.h"

#include <FileSystemModule/FileUtility.h>
#include <FileSystemModule/Filesystem.h>
#include <FileSystemModule/Iterators/DirectoryIterator.h>
#include <FileSystemModule/IOThreads/IOThreads.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreModule/JSON/JSONWriter.h>
#include <CoreModule/JSON/JSONReader.h>

VT_DEFINE_LOG_CATEGORY(LogProject);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(ProjectManager, Default, PreEngine);

	ProjectManager::ProjectManager()
	{
		VT_ASSERT(s_instance == nullptr);
		s_instance = this;
	}

	ProjectManager::~ProjectManager()
	{
		s_instance = nullptr;
	}

	void ProjectManager::Initialize()
	{
		m_pluginRegistry = SubSystemManager::GetSubSystem<PluginRegistry>();
		m_pluginSystem = SubSystemManager::GetSubSystem<PluginSystem>();

		// Get the project from the global command line.
		Filesystem::Path projectFilepath;
		if (GlobalCommandLine::Get().IsArgDefined("project"))
		{
			projectFilepath = GlobalCommandLine::Get().GetArgValue("project");
		}

		LoadProject(projectFilepath);
	}

	void ProjectManager::OnPostStageInitializaton()
	{
		m_pluginSystem->LoadPlugins(*m_currentProject);
	}

	void ProjectManager::LoadProject(const Filesystem::Path& projectPath)
	{
		m_currentProject = CreateUnique<Project>();

		if (!projectPath.IsEmpty())
		{
			m_currentProject->rootDirectory = projectPath.ParentPath();
			m_currentProject->filepath = projectPath;
		}
		else
		{
			m_currentProject->rootDirectory = Filesystem::GetWorkingDirectory();
		
			for (const auto& entry : Filesystem::DirectoryIterator("./"))
			{
				if (entry.path.Extension() == L".vtproj")
				{
					m_currentProject->filepath = entry.path;
					break;
				}
			}
		}

		// Correct working directory should have been setup at this point.
		m_currentEngineDirectory = Filesystem::GetWorkingDirectory();

		m_pluginRegistry->FindAndRegisterPluginsInDirectory(m_currentProject->rootDirectory / "Plugins");
		m_pluginRegistry->FindAndRegisterPluginsInDirectory(m_currentEngineDirectory / "Plugins");

		if (!projectPath.IsEmpty())
		{
			VT_LOGC(Info, LogProject, "Loading project {0}", projectPath);
			DeserializeProject();
		}
		else
		{
			VT_LOGC(Warning, LogProject, "No project filepath provided, not loading any project!");
		}
	}

	void ProjectManager::SerializeProject()
	{
		JSONWriter jsonWriter;
		jsonWriter.BeginDocument();
		jsonWriter.AppendKeyValue("EngineVersion", VT_VERSION.ToString());
		jsonWriter.AppendKeyValue("Name", m_currentProject->name);
		jsonWriter.AppendKeyValue("CompanyName", m_currentProject->companyName);
		jsonWriter.AppendKeyValue("AudioBanksDirectory", m_currentProject->audioDirectory);
		jsonWriter.AppendKeyValue("IconPath", m_currentProject->iconFilepath);
		jsonWriter.AppendKeyValue("CursorPath", m_currentProject->cursorFilepath);
		jsonWriter.AppendKeyValue("StartScenePath", m_currentProject->startSceneFilepath);
		jsonWriter.EndDocument();

		String prettyJson = jsonWriter.GetPrettyJSON();
		FileUtility::WriteStringToFile(m_currentProject->filepath, std::move(prettyJson), true);
	}

	void ProjectManager::DeserializeProject()
	{
		String jsonString;
		if (!FileUtility::ReadStringFromFile(m_currentProject->filepath, jsonString))
		{
			VT_LOGC(Error, LogProject, "Failed to open file: {0}!", m_currentProject->filepath);
			return;
		}

		JSONReader jsonReader;
		if (!jsonReader.Parse(jsonString))
		{
			VT_LOGC(Error, LogProject, "Project file {0} is invalid!", m_currentProject->filepath);
			return;
		}

		String engineVersionStr;
		if (jsonReader.TryGet("EngineVersion", engineVersionStr))
		{
			m_currentProject->engineVersion = engineVersionStr;
		}
		jsonReader.TryGet("Name", m_currentProject->name);
		jsonReader.TryGet("CompanyName", m_currentProject->companyName);
		jsonReader.TryGet("AssetsDirectory", m_currentProject->assetsDirectoryName);
		jsonReader.TryGet("AudioBanksDirectory", m_currentProject->audioDirectory);
		jsonReader.TryGet("IconPath", m_currentProject->iconFilepath);
		jsonReader.TryGet("CursorPath", m_currentProject->cursorFilepath);
		jsonReader.TryGet("StartScenePath", m_currentProject->startSceneFilepath);

		jsonReader.IterateArray("Plugins", [&]() 
		{
			String pluginName;
			jsonReader.Get(pluginName);

			const auto& definition = m_pluginRegistry->GetPluginDefinitionByName(pluginName);
			if (definition.guid != VoltGUID::Null())
			{
				m_currentProject->pluginDefinitions.emplace_back(definition);
			}
			else
			{
				VT_LOGC(Warning, LogProject, "Plugin with name {} does not exist!", pluginName);
			}
		});

		if (!m_currentProject->engineVersion.IsValid() || m_currentProject->engineVersion != VT_VERSION)
		{
			m_currentProject->isDeprecated = true;
			VT_LOGC(Error, LogProject, "The loaded project is deprecated!");
		}

		m_engineAssetsDirectory = m_currentEngineDirectory / "Engine";
		m_currentProject->assetsDirectory = GetProjectDirectory() / GetAssetsDirectoryName();
		m_currentProject->generatedDirectory = GetProjectDirectory() / "Generated";
	}

	const Filesystem::Path ProjectManager::GetAssetsDirectory()
	{
		return s_instance->m_currentProject->isDeprecated ? "./" : s_instance->m_currentProject->assetsDirectory;
	}

	const StringView ProjectManager::GetAssetsDirectoryName()
	{
		return s_instance->m_currentProject->assetsDirectoryName;
	}

	const Filesystem::Path ProjectManager::GetAudioBanksDirectory()
	{
		return s_instance->m_currentProject->isDeprecated ? "./" : s_instance->m_currentProject->rootDirectory / s_instance->m_currentProject->audioDirectory;
	}

	const Filesystem::Path ProjectManager::GetProjectDirectory()
	{
		return s_instance->m_currentProject->isDeprecated ? "./" : s_instance->m_currentProject->rootDirectory;
	}

	const Filesystem::Path ProjectManager::GetEngineRootDirectory()
	{
		return s_instance->m_currentEngineDirectory;
	}

	const Filesystem::Path ProjectManager::GetEngineAssetsDirectory()
	{
		return s_instance->m_engineAssetsDirectory;
	}

	const Filesystem::Path ProjectManager::GetPathRelativeToEngine(const Filesystem::Path& path)
	{
		return Filesystem::Relative(path, s_instance->m_currentEngineDirectory);
	}

	const Filesystem::Path ProjectManager::GetCachePath()
	{
		return GetProjectDirectory() / GetAssetsDirectory() / "Cache";
	}

	const Filesystem::Path ProjectManager::GetGeneratedDirectory()
	{
		return s_instance->m_currentProject->generatedDirectory;
	}

	const Filesystem::Path ProjectManager::GetPathRelativeToProject(const Filesystem::Path& path)
	{
		return Filesystem::Relative(path, GetProjectDirectory());
	}

	const Filesystem::Path ProjectManager::GetOrCreateSettingsDirectory()
	{
		const Filesystem::Path dir = GetRootDirectory() / "Settings";
		if (!Filesystem::Exists(dir))
		{
			Filesystem::CreateDirectories(dir);
		}

		return dir;
	}

	const Filesystem::Path ProjectManager::GetPhysicsSettingsPath()
	{
		return GetOrCreateSettingsDirectory() / "PhysicsSettings.yaml";
	}

	const Filesystem::Path ProjectManager::GetPhysicsLayersPath()
	{
		return GetOrCreateSettingsDirectory() / "PhysicsLayers.yaml";
	}

	const Filesystem::Path& ProjectManager::GetRootDirectory()
	{
		return s_instance->m_currentProject->rootDirectory;
	}

	const bool ProjectManager::IsCurrentProjectDeprecated()
	{
		return s_instance->m_currentProject->isDeprecated;
	}

	const bool ProjectManager::AreCurrentProjectMetaFilesDeprecated()
	{
		return s_instance->m_currentProject->engineVersion < Version::Create(0, 1, 1);
	}

	const Project& ProjectManager::GetProject()
	{
		return *s_instance->m_currentProject;
	}

	void ProjectManager::OnProjectUpgraded()
	{
		s_instance->m_currentProject->isDeprecated = false;
	}

	void ProjectManager::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<PluginSystem>();
		outDependencies.AddDependency<PluginRegistry>();
		outDependencies.AddDependency<IOThreads>();
	}
}
