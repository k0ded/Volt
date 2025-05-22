#pragma once

#include "LogModule/Config.h"
#include "LogModule/LogCommon.h"
#include "LogModule/LogCategory.h"
#include "LogModule/DefaultLoggingCategories.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/UUID.h>

#include <SubSystem/SubSystem.h>

#include <memory>
#include <mutex>
#include <format>
#include <filesystem>
#include <functional>

namespace spdlog
{
	class async_logger;
}

struct LogCallbackData
{
	std::string category;
	std::string message;

	LogVerbosity severity;
};

typedef UUID32 LogCallbackHandle;

class VTLOG_API Log : public SubSystem
{
public:
	Log();
	~Log();

	template<typename LogCategory, typename... Args>
	static void LogFormatted(LogVerbosity severity, const LogCategory& category, const std::string& format, Args&&... args)
	{
		const std::string message = std::vformat(format, std::make_format_args(args...));
		Get().LogMessage(severity, std::string(category.GetName()), message);
	}

	template<typename LogCategory, typename... Args>
	static void LogUnformatted(LogVerbosity severity, const LogCategory& category, const std::string& message)
	{
		Get().LogMessage(severity, std::string(category.GetName()), message);
	}

	LogCallbackHandle RegisterCallback(const std::function<void(const LogCallbackData& callbackData)>& callback);
	void UnregisterCallback(LogCallbackHandle handle);

	void Flush();
	void EnableLogging(bool enable);

	VT_NODISCARD VT_INLINE static Log& Get() { return *s_instance; }

	VT_DECLARE_SUBSYSTEM("{AA12B0EC-2224-4A5E-A274-F6FBEE00B546}"_guid)

private:
	void LogMessage(LogVerbosity severity, const std::string& category, const std::string& message);

	inline static Log* s_instance = nullptr;

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

// Special formatters
namespace std
{
	template <>
	struct formatter<filesystem::path> : formatter<string>
	{
		auto format(filesystem::path p, format_context& ctx) const
		{
			return formatter<string>::format(
			  std::format("{}", p.string()), ctx);
		}
	};
}
