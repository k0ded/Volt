#include "vtcorepch.h"

#include "Volt-Core/Project/ProjectManager.h"
#include "Volt-Core/PluginSystem/PluginRegistry.h"
#include "Volt-Core/Version.h"

#include <CoreUtilities/StringUtility.h>

#include <CoreUtilities/JSON/JSONWriter.h>
#include <CoreUtilities/JSON/JSONReader.h>
#include <CoreUtilities/FileIO/FileUtility.h>

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

	void ProjectManager::LoadProject(const std::filesystem::path projectPath, PluginRegistry& pluginRegistry)
	{
		m_currentProject = CreateScope<Project>();

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

		pluginRegistry.FindAndRegisterPluginsInDirectory(m_currentProject->rootDirectory / "Plugins");
		pluginRegistry.FindAndRegisterPluginsInDirectory(m_currentEngineDirectory / "Plugins");

		if (!projectPath.empty())
		{
			VT_LOGC(Info, LogProject, "Loading project {0}", projectPath);
			DeserializeProject(pluginRegistry);
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

		const std::string prettyJson = jsonWriter.GetPrettyJSON();
		FileUtility::WriteStringToFile(m_currentProject->filepath, prettyJson, true);
	}

	void ProjectManager::DeserializeProject(PluginRegistry& pluginRegistry)
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
			if (!jsonReader.TryGet("Name", pluginName))
			{
				return;
			}

			const auto& definition = pluginRegistry.GetPluginDefinitionByName(pluginName);
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
}
