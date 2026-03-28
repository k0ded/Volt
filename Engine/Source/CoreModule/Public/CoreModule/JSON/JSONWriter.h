#pragma once

#include "CoreModule/JSON/JSONInclude.h"
#include "CoreModule/Config.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Concepts.h>
#include <CoreUtilities/String/VoltString.h>
#include <CoreUtilities/Filesystem/Path.h>

class VTC_API JSONWriter
{
public:
	JSONWriter();

	void BeginDocument();
	void EndDocument();

	void BeginArray(StringView key);
	void EndArray();

	void BeginObject(StringView key = "");
	void EndObject();

	template<typename T> void AppendKeyValue(StringView key, const T& value);
	template<Enum T> void AppendKeyValue(StringView key, const T& value);
	template<typename T> void AppendValue(const T& value);

	template<> void AppendKeyValue(StringView key, const Filesystem::Path& value);
	template<> void AppendKeyValue(StringView key, const float& value);

	String View();
	String GetPrettyJSON();

private:
	nlohmann::json* Current();

	nlohmann::json m_document;
	Vector<nlohmann::json*> m_stack;

	bool m_isDocumentStarted = false;
};

template<typename T>
inline void JSONWriter::AppendKeyValue(StringView key, const T& value)
{
	nlohmann::json& obj = *Current();
	VT_ENSURE(obj.is_object());

	std::string temp(key.data(), key.size());

	if constexpr (std::is_same_v<String, T>)
	{
		std::string tempValue(value.begin(), value.end());
		obj[temp] = tempValue;
	}
	else
	{
		obj[temp] = value;
	}
}

template<Enum T>
inline void JSONWriter::AppendKeyValue(StringView key, const T& value)
{
	using Underlying = std::underlying_type_t<T>;
	AppendKeyValue(key, static_cast<Underlying>(value));
}

template<>
inline void JSONWriter::AppendKeyValue(StringView key, const Filesystem::Path& value)
{
	AppendKeyValue(key, value.ToString());
}

template<>
inline void JSONWriter::AppendKeyValue(StringView key, const float& value)
{
	AppendKeyValue(key, static_cast<double>(value));
}

template<typename T>
inline void JSONWriter::AppendValue(const T& value)
{
	nlohmann::json& arr = *Current();
	VT_ENSURE(arr.is_array());

	if constexpr (std::is_same_v<String, T>)
	{
		std::string tempValue(value.begin(), value.end());
		arr.emplace_back(tempValue);
	}
	else
	{
		arr.emplace_back(value);
	}

	arr.emplace_back(value);
}
