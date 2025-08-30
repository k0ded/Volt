#include "LogCategory.h"

LogCategoryRegistry g_logCategoryRegistry;

LogCategoryBase::LogCategoryBase(std::string_view categoryName, LogVerbosity categoryVerbosity)
	: m_verbosity(categoryVerbosity), m_name(categoryName)
{
}

LogCategoryBase::~LogCategoryBase()
{
}

void LogCategoryRegistry::RegisterLogCategory(LogCategoryBase* category)
{
	auto it = std::find_if(m_registeredCategories.begin(), m_registeredCategories.end(), [category](LogCategoryBase* cat)
	{
		return cat == category || cat->GetName() == category->GetName();
	});

	if (it == m_registeredCategories.end())
	{
		m_registeredCategories.emplace_back(category);
	}
}

void LogCategoryRegistry::UnregisterLogCategory(LogCategoryBase* category)
{
	auto it = std::find_if(m_registeredCategories.begin(), m_registeredCategories.end(), [category](LogCategoryBase* cat)
	{
		return cat == category;
	});

	if (it != m_registeredCategories.end())
	{
		m_registeredCategories.erase_unsorted(it);
	}
}
