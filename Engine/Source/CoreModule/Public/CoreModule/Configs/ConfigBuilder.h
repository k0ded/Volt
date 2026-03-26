#pragma once

#include "CoreModule/Configs/Config.h"

class ConfigBuilder
{
public:
	Config Build();
	ConfigBuilder& Section(const String& sectionName);

	template<typename T>
	ConfigBuilder& Set(const String& key, T value)
	{
		if (VT_CHECK_MSG(m_currentSection, "Values may only live in sections!"))
		{
			m_currentSection->Set(key, ConfigValue(value));
		}

		return *this;
	}

private:
	Config m_config;
	ConfigSection* m_currentSection = nullptr;
};
