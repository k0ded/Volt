#include "Log.h"

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include <CoreUtilities/StringUtility.h>
#include <CoreUtilities/FileSystem.h>

VT_REGISTER_SUBSYSTEM(Log, Minimal, PreEngine, 1);

Log::Log()
{
	VT_ENSURE(s_instance == nullptr);
	s_instance = this;

	spdlog::init_thread_pool(8192, 1);
	spdlog::set_pattern("%^[%T] %n: %v%$");

	std::vector<spdlog::sink_ptr> sinks;

	const std::filesystem::path logDirectory = std::filesystem::current_path() / "Log";
	if (!FileSystem::Exists(logDirectory))
	{
		FileSystem::CreateDirectories(logDirectory);
	}

	sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logDirectory.string() + "/Log.txt", true));

	if (::IsDebuggerPresent())
	{
		sinks.emplace_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
	}

	m_logger = std::make_shared<spdlog::async_logger>("VOLT", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	spdlog::register_logger(m_logger);

	m_logger->set_level(spdlog::level::trace);
}

Log::~Log()
{
	Flush();

	spdlog::shutdown();

	m_logger = nullptr;
	s_instance = nullptr;
}

LogCallbackHandle Log::RegisterCallback(const std::function<void(const LogCallbackData& callbackData)>& callback)
{
	LogCallbackHandle handle = {};
	m_callbacks.emplace_back(callback, handle);
	return handle;
}

void Log::UnregisterCallback(LogCallbackHandle handle)
{
	auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(), [handle](const auto& data)
	{
		return data.handle == handle;
	});

	std::scoped_lock lock{ m_callbackMutex };
	if (it != m_callbacks.end())
	{
		m_callbacks.erase(it);
	}
}

void Log::Flush()
{
	if (m_logger && m_isEnabled)
	{
		m_logger->flush();
	}
}

void Log::EnableLogging(bool enable)
{
	m_isEnabled = enable;
}

void Log::LogMessage(LogVerbosity severity, const std::string& category, const std::string& message)
{
	if (!m_isEnabled)
	{
		return;
	}

	std::string finalString = category.empty() ? "" : "[" + category + "]: ";
	finalString += message;

	switch (severity)
	{
		case LogVerbosity::Trace:
			m_logger->trace(finalString);
			break;
		case LogVerbosity::Info:
			m_logger->info(finalString);
			break;
		case LogVerbosity::Warning:
			m_logger->warn(finalString);
			break;
		case LogVerbosity::Error:
			m_logger->error(finalString);
			break;
		case LogVerbosity::Critical:
			m_logger->critical(finalString);
			break;
	}

	LogCallbackData callbackData{};
	callbackData.category = category;
	callbackData.message = finalString;
	callbackData.severity = severity;

	std::scoped_lock lock{ m_callbackMutex };
	for (const auto& data : m_callbacks)
	{
		data.callbackFunc(callbackData);
	}
}
