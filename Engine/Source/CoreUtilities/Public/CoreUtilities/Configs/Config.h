#pragma once

#include "CoreUtilities/Variant.h"
#include "CoreUtilities/Containers/Map.h"

#include <string>


class ConfigValue
{
public:
	using ValueType = Variant<int32_t, float, bool, std::string>;

	ConfigValue() = default;
	~ConfigValue() = default;

	VTCOREUTIL_API std::string ToString() const;

	template<typename T>
	ConfigValue(T val)
		: m_value(val)
	{}

	template<typename T>
	T Get() const
	{
		return m_value.Get<T>();
	}

private:
	ValueType m_value;
};

class ConfigSection
{
public:
	VTCOREUTIL_API void Set(const std::string& key, ConfigValue value);

	VT_INLINE const Map<std::string, ConfigValue>& GetValues() const { return m_sectionValues; }

private:
	Map<std::string, ConfigValue> m_sectionValues;
};

class Config
{
public:
	// Appends another config to this config.
	VTCOREUTIL_API void Append(const Config& otherConfig);
	VTCOREUTIL_API ConfigSection& Section(const std::string& name);
	VTCOREUTIL_API std::string ToString();
	VTCOREUTIL_API const ConfigValue* TryGetValue(const std::string& sectionName, const std::string& key) const;

	VT_INLINE const Map<std::string, ConfigSection>& GetSections() const { return m_sections; }

private:
	Map<std::string, ConfigSection> m_sections;
};
