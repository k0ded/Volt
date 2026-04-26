#include "cpch.h"
#include "CoreModule/ConfigManager.h"
#include "CoreModule/Project/ProjectManager.h"
#include "CoreModule/Configs/ConfigParser.h"

#include <FileSystemModule/FileUtility.h>
#include <FileSystemModule/Filesystem.h>

#include <CoreUtilities/ConsoleVariableRegistry.h>

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
		outDependencies.AddDependency<ProjectManager>();
	}

	const ConfigValue* ConfigManager::TryGetConfigValue(const String& sectionName, const String& key) const
	{
		return m_combinedConfig.TryGetValue(sectionName, key);
	}

	void ConfigManager::LoadConfigs()
	{
		// Engine base config
		const Filesystem::Path engineConfigFilepath = Filesystem::GetWorkingDirectory() / "Config" / "Engine.ini";

		Config tempConfig;
		if (LoadConfig(engineConfigFilepath, tempConfig))
		{
			m_combinedConfig.Append(tempConfig);
		}

		// Project engine config
		const Filesystem::Path gameConfigFilepath = ProjectManager::GetProjectDirectory() / "Config" / "Engine.ini";
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
			Ref<RegisteredConsoleVariableBase> refConsoleVar = consoleVar.Lock();

			if (refConsoleVar->IsFloat())
			{
				if (value.Is<float>())
				{
					const float tempVal = value.Get<float>();
					refConsoleVar->SetOverride(&tempVal);
				}
				else if (value.Is<int32_t>())
				{
					const float tempVal = static_cast<float>(value.Get<int32_t>());
					refConsoleVar->SetOverride(&tempVal);
				}
			}
			else if (refConsoleVar->IsInteger())
			{
				if (value.Is<float>())
				{
					const int32_t tempVal = static_cast<int32_t>(value.Get<float>());
					refConsoleVar->SetOverride(&tempVal);
				}
				else if (value.Is<int32_t>())
				{
					const int32_t tempVal = value.Get<int32_t>();
					refConsoleVar->SetOverride(&tempVal);
				}
			}
			else if (refConsoleVar->IsString())
			{
				if (value.Is<String>())
				{
					const String tempVal = value.Get<String>();
					refConsoleVar->SetOverride(&tempVal);
				}
			}
		}
	}

	bool ConfigManager::LoadConfig(const Filesystem::Path& configFilepath, Config& outConfig)
	{
		String configStr;
		if (!FileUtility::ReadStringFromFile(configFilepath, configStr))
		{
			VT_LOGC(Warning, LogConfigManager, "Failed to find config at filepath {}!", configFilepath);
			return false;
		}

		outConfig = ConfigParser::ParseConfigFromString(configStr);
		return true;
	}
}
