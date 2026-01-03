#include "vtcorepch.h"

#include "Volt-Core/ConfigManager.h"
#include "Volt-Core/Project/ProjectManager.h"

#include <CoreUtilities/Configs/ConfigParser.h>
#include <CoreUtilities/FileIO/FileUtility.h>
#include <CoreUtilities/FileSystem.h>

VT_DEFINE_LOG_CATEGORY(LogConfigManager);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(ConfigManager, Minimal, PreEngine);

	void ConfigManager::Initialize()
	{
		LoadConfigs();
	}

	void ConfigManager::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<Log>();
		outDependencies.AddDependency<ProjectManager>();
	}

	const ConfigValue* ConfigManager::TryGetConfigValue(const std::string& sectionName, const std::string& key) const
	{
		return m_combinedConfig.TryGetValue(sectionName, key);
	}

	void ConfigManager::LoadConfigs()
	{
		// Engine base config
		const std::filesystem::path engineConfigFilepath = std::filesystem::current_path() / "Config" / "Engine.ini";

		Config tempConfig;
		if (LoadConfig(engineConfigFilepath, tempConfig))
		{
			m_combinedConfig.Append(tempConfig);
		}

		// Project engine config
		const std::filesystem::path gameConfigFilepath = ProjectManager::GetProjectDirectory() / "Config" / "Engine.ini";
		if (LoadConfig(gameConfigFilepath, tempConfig))
		{
			m_combinedConfig.Append(tempConfig);
		}
	}

	bool ConfigManager::LoadConfig(const std::filesystem::path& configFilepath, Config& outConfig)
	{
		std::string configStr;
		if (!FileUtility::ReadStringFromFile(configFilepath, configStr))
		{
			VT_LOGC(Warning, LogConfigManager, "Failed to find config at filepath {}!", configFilepath);
			return false;
		}

		outConfig = ConfigParser::ParseConfigFromString(configStr);
		return true;
	}
}
