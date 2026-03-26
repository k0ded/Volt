#pragma once

#include "CoreModule/Config.h"

#include <CoreUtilities/Variant.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/String/VoltString.h>

class ConfigValue
{
public:
	using ValueType = Variant<int32_t, float, bool, String>;

	ConfigValue() = default;
	~ConfigValue() = default;

	VTC_API String ToString() const;

	template<typename T>
	ConfigValue(T val)
		: m_value(val)
	{}

	template<typename T>
	T Get() const
	{
		return m_value.Get<T>();
	}

	template<typename T>
	bool Is() const
	{
		return m_value.Is<T>();
	}

private:
	ValueType m_value;
};

class ConfigSection
{
public:
	VTC_API void Set(const String& key, ConfigValue value);

	VT_INLINE const Map<String, ConfigValue>& GetValues() const { return m_sectionValues; }

private:
	Map<String, ConfigValue> m_sectionValues;
};

class Config
{
public:
	// Appends another config to this config.
	VTC_API void Append(const Config& otherConfig);
	VTC_API ConfigSection& Section(const String& name);
	VTC_API String ToString();
	VTC_API const ConfigValue* TryGetValue(const String& sectionName, const String& key) const;

	VT_INLINE const Map<String, ConfigSection>& GetSections() const { return m_sections; }

private:
	Map<String, ConfigSection> m_sections;
};
