#include "cupch.h"

#include "CoreUtilities/Configs/Config.h"

std::string ConfigValue::ToString() const
{
	if (m_value.Is<int32_t>())
	{
		return std::to_string(m_value.Get<int32_t>());
	}
	else if (m_value.Is<float>())
	{
		return std::to_string(m_value.Get<float>());
	}
	else if (m_value.Is<bool>())
	{
		return m_value.Get<bool>() ? "true" : "false";
	}
	else
	{
		const std::string& val = m_value.Get<std::string>();

		// Check if quotes are required.
		if (val.find_first_of(" \t;#") != std::string::npos)
		{
			return "\"" + val + "\"";
		}

		return val;
	}
}

void Config::Append(const Config& otherConfig)
{
	for (const auto& [sectionName, section] : otherConfig.GetSections())
	{
		if (m_sections.contains(sectionName))
		{
			for (const auto& [key, value] : section.GetValues())
			{
				m_sections[sectionName].Set(key, value);
			}
		}
		else
		{
			m_sections[sectionName] = section;
		}
	}
}

ConfigSection& Config::Section(const std::string& name)
{
	return m_sections[name];
}

std::string Config::ToString()
{
	std::stringstream sstream;

	for (const auto& [sectionName, section] : m_sections)
	{
		sstream << "[" << sectionName << "]\n";

		for (const auto& [key, value] : section.GetValues())
		{
			sstream << key << " = " << value.ToString() << "\n";
		}

		sstream << "\n";
	}

	return sstream.str();
}

const ConfigValue* Config::TryGetValue(const std::string& sectionName, const std::string& key) const
{
	auto it = m_sections.find(sectionName);
	if (it == m_sections.end())
	{
		return nullptr;
	}

	const auto& values = (*it).second.GetValues();

	auto valueIt = values.find(key);
	if (valueIt == values.end())
	{
		return nullptr;
	}

	return &(*valueIt).second;
}

void ConfigSection::Set(const std::string& key, ConfigValue value)
{
	m_sectionValues[key] = value;
}
