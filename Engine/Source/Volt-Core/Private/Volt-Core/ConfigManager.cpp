#include "vtcorepch.h"

#include "Volt-Core/ConfigManager.h"
#include "Volt-Core/Project/ProjectManager.h"
#include "Volt-Core/Console/ConsoleVariableRegistry.h"

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
		SetupConsoleVariables();
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

	void ConfigManager::SetupConsoleVariables()
	{
		// We only recognize console variables under the "ConsoleVariables" section
		if (!m_combinedConfig.GetSections().contains("ConsoleVariables"))
		{
			return;
		}

		const ConfigSection& consoleVarsSection = m_combinedConfig.GetSections().at("ConsoleVariables");

		for (const auto& [varName, value] : consoleVarsSection.GetValues())
		{
			if (!ConsoleVariableRegistry::Get().VariableExists(varName))
			{
				continue;
			}

			Weak<RegisteredConsoleVariableBase> consoleVar = ConsoleVariableRegistry::Get().GetVariable(varName);
		
			if (consoleVar->IsFloat())
			{
				if (value.Is<float>())
				{
					const float tempVal = value.Get<float>();
					consoleVar->Set(&tempVal);
				}
				else if (value.Is<int32_t>())
				{
					const float tempVal = static_cast<float>(value.Get<int32_t>());
					consoleVar->Set(&tempVal);
				}
			}
			else if (consoleVar->IsInteger())
			{
				if (value.Is<float>())
				{
					const int32_t tempVal = static_cast<int32_t>(value.Get<float>());
					consoleVar->Set(&tempVal);
				}
				else if (value.Is<int32_t>())
				{
					const int32_t tempVal = value.Get<int32_t>();
					consoleVar->Set(&tempVal);
				}
			}
			else if (consoleVar->IsString())
			{
				if (value.Is<std::string>())
				{
					const std::string tempVal = value.Get<std::string>();
					consoleVar->Set(&tempVal);
				}
			}
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
