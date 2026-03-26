#include "CoreModule/JSON/JSONReader.h"

JSONReader::JSONReader()
	: m_isIteratingArray(false)
{}

bool JSONReader::Parse(StringView string)
{
	m_document = nlohmann::json::parse(string, nullptr, false);
	return !m_document.is_discarded() && m_document.is_object();
}

bool JSONReader::OpenObject(StringView key)
{
	const nlohmann::json* current = GetCurrentObject();

	std::string temp(key.data(), key.size());

	auto it = current->find(temp);
	if (it == current->end() || !it->is_object())
	{
		return false;
	}

	m_objectStack.push_back(&(*it));
	return true;
}

void JSONReader::CloseObject()
{
	VT_ENSURE(!m_objectStack.empty());
	m_objectStack.pop_back();
}

const nlohmann::json* JSONReader::GetCurrentObject() const
{
	if (m_objectStack.empty())
	{
		return &m_document;
	}

	return m_objectStack.back();
}

std::string JSONReader::GetKey(StringView key) const
{
	return std::string(key.data(), key.size());
}
