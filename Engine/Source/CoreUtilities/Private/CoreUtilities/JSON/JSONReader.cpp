#include "cupch.h"

#include "CoreUtilities/JSON/JSONReader.h"

JSONReader::JSONReader()
	: m_isIteratingArray(false)
{}

bool JSONReader::Parse(std::string_view string)
{
	m_document = nlohmann::json::parse(string, nullptr, false);
	return !m_document.is_discarded() && m_document.is_object();
}

bool JSONReader::OpenObject(std::string_view key)
{
	const nlohmann::json* current = GetCurrentObject();

	auto it = current->find(key);
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
