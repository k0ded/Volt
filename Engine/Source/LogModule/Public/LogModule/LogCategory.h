#pragma once

#include "LogModule/Config.h"
#include "LogModule/LogCommon.h"

#include <CoreUtilities/CompilerTraits.h>
#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/String/StringView.h>

class VTLOG_API LogCategoryBase
{
public:
	LogCategoryBase(StringView categoryName, LogVerbosity categoryVerbosity);
	~LogCategoryBase();

	VT_INLINE constexpr StringView GetName() const { return m_name; }
	VT_INLINE constexpr LogVerbosity GetVerbosity() const { return m_verbosity; }

private:
	LogVerbosity m_verbosity;
	const StringView m_name;
};

template<LogVerbosity verbosity>
class VTLOG_API LogCategory : public LogCategoryBase
{
public:
	VT_INLINE LogCategory(StringView categoryName)
		: LogCategoryBase(categoryName, verbosity)
	{ }
};

class VTLOG_API LogCategoryRegistry
{
public:
	void RegisterLogCategory(LogCategoryBase* category);
	void UnregisterLogCategory(LogCategoryBase* category);

	VT_NODISCARD VT_INLINE const Vector<LogCategoryBase*> GetRegisteredLogCategories() const { return m_registeredCategories; }

	static LogCategoryRegistry& Get();

private:
	Vector<LogCategoryBase*> m_registeredCategories;
};

#define VT_DECLARE_LOG_CATEGORY(categoryName, verbosity) \
	extern class LogCategory##categoryName : public LogCategory<verbosity> \
	{ \
	public: \
		VT_INLINE LogCategory##categoryName() : LogCategory(#categoryName) \
		{ \
			LogCategoryRegistry::Get().RegisterLogCategory(this); \
		} \
		VT_INLINE ~LogCategory##categoryName() \
		{ \
			LogCategoryRegistry::Get().UnregisterLogCategory(this); \
		} \
	} categoryName

#define VT_DECLARE_LOG_CATEGORY_EXPORT(exportKeyword, categoryName, verbosity) \
	extern class exportKeyword LogCategory##categoryName : public LogCategory<verbosity> \
	{ \
	public: \
		VT_INLINE LogCategory##categoryName() : LogCategory(#categoryName) \
		{ \
			LogCategoryRegistry::Get().RegisterLogCategory(this); \
		} \
		VT_INLINE ~LogCategory##categoryName() \
		{ \
			LogCategoryRegistry::Get().UnregisterLogCategory(this); \
		} \
	} exportKeyword categoryName

#define VT_DEFINE_LOG_CATEGORY(categoryName) LogCategory##categoryName categoryName
