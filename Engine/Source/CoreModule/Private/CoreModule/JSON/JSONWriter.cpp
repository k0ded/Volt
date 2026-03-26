#include "CoreModule/JSON/JSONWriter.h"
#include "CoreModule/JSON/CommonJSONSerialization.h"

#include <CoreUtilities/VoltAssert.h>

JSONWriter::JSONWriter() = default;

void JSONWriter::BeginDocument()
{
	VT_ENSURE_MSG(!m_isDocumentStarted, "Document may only be begun once!");

	m_document = nlohmann::json::object();
	m_stack.clear();
	m_stack.push_back(&m_document);

	m_isDocumentStarted = true;
}

void JSONWriter::EndDocument()
{
	VT_ENSURE_MSG(m_isDocumentStarted, "Document has not been started!");
	VT_ENSURE_MSG(m_stack.size() == 1, "Missing EndArray or EndObject!");
}

nlohmann::json* JSONWriter::Current()
{
	VT_ENSURE(!m_stack.empty());
	return m_stack.back();
}

void JSONWriter::BeginArray(StringView key)
{
	std::string temp(key.data(), key.size());

	nlohmann::json& parent = *Current();
	nlohmann::json& arr = parent[temp];
	arr = nlohmann::json::array();

	m_stack.push_back(&arr);
}

void JSONWriter::EndArray()
{
	VT_ENSURE(Current()->is_array());
	m_stack.pop_back();
}

void JSONWriter::BeginObject(StringView key)
{
	nlohmann::json* obj = nullptr;

	if (!key.empty())
	{
		std::string temp(key.data(), key.size());

		nlohmann::json& parent = *Current();
		obj = &parent[temp];
		*obj = nlohmann::json::object();
	}
	else
	{
		nlohmann::json& parent = *Current();
		parent.emplace_back(nlohmann::json::object());
		obj = &parent.back();
	}

	m_stack.push_back(obj);
}

void JSONWriter::EndObject()
{
	VT_ENSURE(Current()->is_object());
	m_stack.pop_back();
}

String JSONWriter::View()
{
	VT_ENSURE(m_isDocumentStarted);
	std::string temp = m_document.dump();

	return String(temp.data(), temp.size());
}

String JSONWriter::GetPrettyJSON()
{
	VT_ENSURE(m_isDocumentStarted);

	std::string temp = m_document.dump(4);
	return String(temp.data(), temp.size());
}
