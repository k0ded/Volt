#pragma once

#include "Volt-Core/Config.h"

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <LogModule/LogCategory.h>

#include <CoreModule/Configs/Config.h>

#include <CoreUtilities/Filesystem/Path.h>

VT_DECLARE_LOG_CATEGORY(LogConfigManager, LogVerbosity::Trace);

namespace Volt
{
	class ConfigManager : public SubSystem
	{
	public:
		void Initialize() override;

		VTCORE_API const ConfigValue* TryGetConfigValue(const String& sectionName, const String& key) const;

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{9B995954-5566-4676-8F13-F1958775679A}"_guid);

	private:
		void LoadConfigs();
		void SetupConsoleVariables();

		bool LoadConfig(const Filesystem::Path& configFilepath, Config& outConfig);

		Config m_combinedConfig;
	};
}
