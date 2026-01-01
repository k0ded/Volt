#pragma once

#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Config.h"
#include "CoreUtilities/Concepts.h"
#include "CoreUtilities/JSON/JSONInclude.h"

#include <filesystem>

class VTCOREUTIL_API JSONWriter
{
public:
	JSONWriter();

	void BeginDocument();
	void EndDocument();

	void BeginArray(std::string_view key);
	void EndArray();

	void BeginObject(std::string_view key = "");
	void EndObject();

	template<typename T> void AppendKeyValue(std::string_view key, const T& value);
	template<Enum T> void AppendKeyValue(std::string_view key, const T& value);
	template<typename T> void AppendValue(const T& value);

	template<> void AppendKeyValue(std::string_view key, const std::filesystem::path& value);
	template<> void AppendKeyValue(std::string_view key, const float& value);

	std::string View();
	std::string GetPrettyJSON();

private:
	nlohmann::json* Current();

	nlohmann::json m_document;
	Vector<nlohmann::json*> m_stack;

	bool m_isDocumentStarted = false;
};

template<typename T>
void JSONWriter::AppendKeyValue(std::string_view key, const T& value)
{
	nlohmann::json& obj = *Current();
	VT_ENSURE(obj.is_object());

	obj[std::string(key)] = value;
}

template<Enum T>
void JSONWriter::AppendKeyValue(std::string_view key, const T& value)
{
	using Underlying = std::underlying_type_t<T>;
	AppendKeyValue(key, static_cast<Underlying>(value));
}

template<>
void JSONWriter::AppendKeyValue(std::string_view key, const std::filesystem::path& value)
{
	AppendKeyValue(key, value.string());
}

template<>
void JSONWriter::AppendKeyValue(std::string_view key, const float& value)
{
	AppendKeyValue(key, static_cast<double>(value));
}

template<typename T>
void JSONWriter::AppendValue(const T& value)
{
	nlohmann::json& arr = *Current();
	VT_ENSURE(arr.is_array());

	arr.emplace_back(value);
}
