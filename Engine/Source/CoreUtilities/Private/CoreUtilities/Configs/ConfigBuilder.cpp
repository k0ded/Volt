#include "cupch.h"

#include "CoreUtilities/Configs/ConfigBuilder.h"

ConfigBuilder& ConfigBuilder::Section(const std::string& sectionName)
{
	m_currentSection = &m_config.Section(sectionName);
	return *this;
}

Config ConfigBuilder::Build()
{
	return std::move(m_config);
}
