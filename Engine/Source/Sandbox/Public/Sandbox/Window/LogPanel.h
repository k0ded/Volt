#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <CoreUtilities/Containers/Array.h>

class LogPanel : public EditorWindow
{
public:
	LogPanel();
	~LogPanel() override;

	void UpdateMainContent() override;

private:
	struct LogCategoryData
	{
		const LogCategoryBase* category;
		bool isActive;
	};

	void RenderTopBar();
	void RenderLogChildWindow();
	void RenderBottomBar();
	void SetupLogCategories();

	void TryAppendLogMessage(const LogCallbackData& logData);
	bool DoesLogEntryPassFilters(const LogCallbackData& logData);
	void RefilterLogMessages();

	uint32_t m_maxMessages = 1000;
	Vector<LogCallbackData> m_logMessages;
	Vector<LogCallbackData> m_filteredLogMessages;

	Vector<LogCategoryData> m_logCategories;
	Array<bool, static_cast<size_t>(LogVerbosity::Critical) + 1> m_logVerbosityActive;

	std::string m_logSearchQuery;

	LogCallbackHandle m_callbackHandle = 0;
	std::mutex m_logMutex;
};
