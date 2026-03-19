#include "vtcorepch.h"

#include "Volt-Core/Project/ProjectManager.h"
#include "Volt-Core/PluginSystem/PluginSystem.h"
#include "Volt-Core/PluginSystem/PluginRegistry.h"
#include "Volt-Core/Version.h"
#include "Volt-Core/GlobalCommandLine.h"

#include <Volt-FileSystem/FileUtility.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/JSON/JSONWriter.h>
#include <CoreUtilities/JSON/JSONReader.h>

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
		std::filesystem::path projectFilepath;
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

	void ProjectManager::LoadProject(const std::filesystem::path projectPath)
	{
		m_currentProject = CreateUnique<Project>();

		if (!projectPath.empty())
		{
			m_currentProject->rootDirectory = projectPath.parent_path();
			m_currentProject->filepath = projectPath;
		}
		else
		{
			m_currentProject->rootDirectory = std::filesystem::current_path();
		
			for (const auto& dir : std::filesystem::directory_iterator("./"))
			{
				if (dir.path().extension() == ".vtproj")
				{
					m_currentProject->filepath = dir.path();
					break;
				}
			}
		}

		// Correct working directory should have been setup at this point.
		m_currentEngineDirectory = std::filesystem::current_path();

		m_pluginRegistry->FindAndRegisterPluginsInDirectory(m_currentProject->rootDirectory / "Plugins");
		m_pluginRegistry->FindAndRegisterPluginsInDirectory(m_currentEngineDirectory / "Plugins");

		if (!projectPath.empty())
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

		std::string prettyJson = jsonWriter.GetPrettyJSON();
		FileUtility::WriteStringToFile(m_currentProject->filepath, std::move(prettyJson), true);
	}

	void ProjectManager::DeserializeProject()
	{
		std::string jsonString;
		if (!FileUtility::ReadStringFromFile(m_currentProject->filepath, jsonString))
		{
			VT_LOGC(Error, LogProject, "Failed to open file: {0}!", m_currentProject->filepath.string());
			return;
		}

		JSONReader jsonReader;
		if (!jsonReader.Parse(jsonString))
		{
			VT_LOGC(Error, LogProject, "Project file {0} is invalid!", m_currentProject->filepath.string());
			return;
		}

		std::string engineVersionStr;
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
			std::string pluginName;
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
	}

	const std::filesystem::path ProjectManager::GetAssetsDirectory()
	{
		return s_instance->m_currentProject->isDeprecated ? "./" : GetProjectDirectory() / GetAssetsDirectoryName();
	}

	const std::string_view ProjectManager::GetAssetsDirectoryName()
	{
		return s_instance->m_currentProject->assetsDirectoryName;
	}

	const std::filesystem::path ProjectManager::GetAudioBanksDirectory()
	{
		return s_instance->m_currentProject->isDeprecated ? "./" : s_instance->m_currentProject->rootDirectory / s_instance->m_currentProject->audioDirectory;
	}

	const std::filesystem::path ProjectManager::GetProjectDirectory()
	{
		return s_instance->m_currentProject->isDeprecated ? "./" : s_instance->m_currentProject->rootDirectory;
	}

	const std::filesystem::path ProjectManager::GetEngineRootDirectory()
	{
		return s_instance->m_currentEngineDirectory;
	}

	const std::filesystem::path ProjectManager::GetEngineAssetsDirectory()
	{
		return s_instance->m_currentEngineDirectory / "Engine";
	}

	const std::filesystem::path ProjectManager::GetPathRelativeToEngine(const std::filesystem::path& path)
	{
		return std::filesystem::relative(path, s_instance->m_currentEngineDirectory);
	}

	const std::filesystem::path ProjectManager::GetCachePath()
	{
		return GetProjectDirectory() / GetAssetsDirectory() / "Cache";
	}

	const std::filesystem::path ProjectManager::GetPathRelativeToProject(const std::filesystem::path& path)
	{
		return std::filesystem::relative(path, GetProjectDirectory());
	}

	const std::filesystem::path ProjectManager::GetOrCreateSettingsDirectory()
	{
		const std::filesystem::path dir = GetRootDirectory() / "Settings";
		if (!std::filesystem::exists(dir))
		{
			std::filesystem::create_directories(dir);
		}

		return dir;
	}

	const std::filesystem::path ProjectManager::GetPhysicsSettingsPath()
	{
		return GetOrCreateSettingsDirectory() / "PhysicsSettings.yaml";
	}

	const std::filesystem::path ProjectManager::GetPhysicsLayersPath()
	{
		return GetOrCreateSettingsDirectory() / "PhysicsLayers.yaml";
	}

	const std::filesystem::path& ProjectManager::GetRootDirectory()
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
		outDependencies.AddDependency<Log>();
	}
}
