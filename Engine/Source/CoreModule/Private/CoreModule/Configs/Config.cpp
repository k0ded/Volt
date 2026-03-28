#include "cpch.h"
#include "CoreModule/Configs/Config.h"

#include <CoreUtilities/String/StringBuilder.h>
#include <CoreUtilities/String/StringFormat.h>

String ConfigValue::ToString() const
{
	if (m_value.Is<int32_t>())
	{
		return FormatString("{}", m_value.Get<int32_t>());
	}
	else if (m_value.Is<float>())
	{
		return FormatString("{}", m_value.Get<float>());
	}
	else if (m_value.Is<bool>())
	{
		return m_value.Get<bool>() ? "true" : "false";
	}
	else
	{
		const String& val = m_value.Get<String>();

		// Check if quotes are required.
		if (val.find_first_of(" \t;#") != String::npos)
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

ConfigSection& Config::Section(const String& name)
{
	return m_sections[name];
}

String Config::ToString()
{
	StringBuilder builder;

	for (const auto& [sectionName, section] : m_sections)
	{
		builder << "[" << sectionName << "]\n";

		for (const auto& [key, value] : section.GetValues())
		{
			builder << key << " = " << value.ToString() << "\n";
		}

		builder << "\n";
	}

	return builder.Get();
}

const ConfigValue* Config::TryGetValue(const String& sectionName, const String& key) const
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

void ConfigSection::Set(const String& key, ConfigValue value)
{
	m_sectionValues[key] = value;
}
