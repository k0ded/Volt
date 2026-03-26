#pragma once

#include "LogModule/Config.h"
#include "LogModule/LogCommon.h"
#include "LogModule/LogCategory.h"
#include "LogModule/DefaultLoggingCategories.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/UUID.h>
#include <CoreUtilities/String/StringFormat.h>

#include <memory>
#include <mutex>
#include <functional>

namespace spdlog
{
	class async_logger;
}

struct LogCallbackData
{
	const LogCategoryBase* category;
	String message;

	LogVerbosity severity;
};

typedef UUID32 LogCallbackHandle;

class VTLOG_API Log
{
public:
	Log();
	~Log();

	template<typename LogCategory, typename... Args>
	static void LogFormatted(LogVerbosity severity, const LogCategory& category, const String& format, Args&&... args)
	{
		std::string_view formatView(format.begin(), format.length());

		const String message = VFormatString(formatView, std::make_format_args(args...));
		Get().LogMessage(severity, &category, message);
	}

	template<typename LogCategory, typename... Args>
	static void LogUnformatted(LogVerbosity severity, const LogCategory& category, const String& message)
	{
		Get().LogMessage(severity, &category, message);
	}

	LogCallbackHandle RegisterCallback(const std::function<void(const LogCallbackData& callbackData)>& callback);
	void UnregisterCallback(LogCallbackHandle handle);

	void Flush();
	void EnableLogging(bool enable);

	VT_NODISCARD static Log& Get();

private:
	void LogMessage(LogVerbosity severity, const LogCategoryBase* category, const String& message);

	std::shared_ptr<spdlog::async_logger> m_logger;

	struct CallbackData
	{
		std::function<void(const LogCallbackData& callbackData)> callbackFunc;
		LogCallbackHandle handle;
	};

	std::mutex m_callbackMutex;
	Vector<CallbackData> m_callbacks;
	bool m_isEnabled = true;
};

#define VT_LOGC(verbosity, category, format, ...) ::Log::LogFormatted(LogVerbosity::verbosity, category, format, __VA_ARGS__)
#define VT_LOG(verbosity, format, ...) ::Log::LogFormatted(LogVerbosity::verbosity, LogTemp, format, __VA_ARGS__)

#define VT_LOGC_UNFORMATTED(verbosity, category, message) ::Log::LogUnformatted(LogVerbosity::verbosity, category, message)
#define VT_LOG_UNFORMATTED(verbosity, message) ::Log::LogUnformatted(LogVerbosity::verbosity, LogTemp, message)
