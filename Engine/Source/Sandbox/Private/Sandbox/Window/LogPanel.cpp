#include "sbpch.h"
#include "Window/LogPanel.h"

#include "Sandbox/Utility/EditorSearchBar.h"
#include "Sandbox/Utility/Theme.h"
#include "Sandbox/Utility/EditorUtilities.h"

#include <Volt-Application/UI/UIUtility.h>
#include <Volt-Application/UI/UIScopedHelpers.h>

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Utility
{
	ImVec4 GetColorFromLevel(LogVerbosity aLevel)
	{
		switch (aLevel)
		{
			case LogVerbosity::Trace: return ImVec4(0.83f, 0.83f, 0.83f, 1.f);
			case LogVerbosity::Info: return ImVec4(1.f, 1.f, 1.f, 1.f);
			case LogVerbosity::Warning: return ImVec4(1.f, 0.92f, 0.21f, 1.f);
			case LogVerbosity::Error: return ImVec4(1.f, 0.f, 0.f, 1.f);
			case LogVerbosity::Critical: return ImVec4(1.f, 0.f, 0.f, 1.f);
		}

		return ImVec4(1.f, 1.f, 1.f, 1.f);
	}
}

LogPanel::LogPanel()
	: EditorWindow("Output Log")
{
	Open();

	m_windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	m_callbackHandle = Log::Get().RegisterCallback([&](const LogCallbackData& message)
	{
		//std::scoped_lock lock{ m_logMutex };
		m_logMessages.emplace_back(message);
		if (m_logMessages.size() >= m_maxMessages)
		{
			m_logMessages.erase(m_logMessages.begin());
		}

		TryAppendLogMessage(message);
	});

	SetupLogCategories();

	for (size_t i = 0; i < m_logVerbosityActive.size(); ++i)
	{
		m_logVerbosityActive[i] = true;
	}
}

LogPanel::~LogPanel()
{
	Log::Get().UnregisterCallback(m_callbackHandle);
	m_callbackHandle = 0;
}

void LogPanel::UpdateMainContent()
{
	RenderTopBar();
	RenderLogChildWindow();
	RenderBottomBar();
}

void LogPanel::RenderTopBar()
{
	constexpr float TopBarHeight = 30.f;

	UI::ScopedColor childColor{ ImGuiCol_ChildBg, { 0.2f, 0.2f, 0.2f, 1.f } };

	if (ImGui::BeginChild("##topBar", { ImGui::GetContentRegionAvail().x, TopBarHeight }))
	{
		static bool active = false;

		static EditorSearchBar searchBar({ 0.2f, 0.2f, 0.2f, 1.f }, false);
		if (searchBar.Render(200.f))
		{
			RefilterLogMessages();
		}

		m_logSearchQuery = searchBar.GetSearchQuery();

		ImGui::SameLine();

		if (ImGui::BeginChild("##filtersChild", { 200.f, 0.f }, ImGuiChildFlags_None, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Filters"))
				{
					bool requiresUpdate = false;

					if (ImGui::BeginMenu("Categories"))
					{
						for (LogCategoryData& category : m_logCategories)
						{
							requiresUpdate |= ImGui::Checkbox(category.category->GetName().data(), &category.isActive);
						}

						ImGui::EndMenu();
					}

					static const std::vector<const char*> logVerbosityNames =
					{
						"Trace",
						"Info",
						"Warning",
						"Error",
						"Critical"
					};

					for (size_t i = 0; i < logVerbosityNames.size(); ++i)
					{
						requiresUpdate |= ImGui::Checkbox(logVerbosityNames[i], &m_logVerbosityActive[i]);
					}

					ImGui::EndMenu();
				
					if (requiresUpdate)
					{
						RefilterLogMessages();
					}
				}

				ImGui::EndMenuBar();
			}
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
}

void LogPanel::RenderLogChildWindow()
{
	VT_PROFILE_FUNCTION();

	constexpr float BottomBarHeight = 30.f;
	const float height = ImGui::GetContentRegionAvail().y - BottomBarHeight;

	UI::ScopedColor childColor{ ImGuiCol_ChildBg, EditorTheme::DarkGreyBackground };

	if (ImGui::BeginChild("##logChild", ImVec2{ ImGui::GetContentRegionAvail().x, height }))
	{
		for (const auto& msg : m_filteredLogMessages)
		{
			ImVec4 color = Utility::GetColorFromLevel(msg.severity);

			ImGui::PushStyleColor(ImGuiCol_Text, Utility::GetColorFromLevel(msg.severity));
			ImGui::TextWrapped("%s", msg.message.c_str());
			ImGui::PopStyleColor();
		}

		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		{
			ImGui::SetScrollHereY(1.f);
		}
	}
	ImGui::EndChild();
}

void LogPanel::RenderBottomBar()
{
	constexpr float BottomBarHeight = 30.f;

	UI::ScopedColor childColor{ ImGuiCol_ChildBg, { 0.2f, 0.2f, 0.2f, 1.f } };

	if (ImGui::BeginChild("##bottomBar", { ImGui::GetContentRegionAvail().x, BottomBarHeight }))
	{
		static std::string query;

		ImGui::PushItemWidth(350.f);
		UI::ShiftCursor(5.f, 4.f);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);

		if (UI::InputTextWithHint("", query, "Enter Console Command", ImGuiInputTextFlags_EnterReturnsTrue))
		{
			auto strings = Utility::SplitStringsByCharacter(query, ' ');
			if (!strings.empty())
			{
				if (Volt::ConsoleVariableRegistry::VariableExists(strings[0]))
				{
					Weak<Volt::RegisteredConsoleVariableBase> weakVariable = Volt::ConsoleVariableRegistry::GetVariable(strings[0]);
					Ref<Volt::RegisteredConsoleVariableBase> variable = weakVariable.Lock();

					std::string message = std::string(variable->GetName()) + " = ";

					if (strings.size() > 1)
					{
						if (variable->IsFloat())
						{
							const float value = std::stof(strings[1]);
							variable->Set(&value);
						}
						else if (variable->IsInteger())
						{
							const int32_t value = std::stoi(strings[1]);
							variable->Set(&value);
						}
						else if (variable->IsString())
						{
							variable->Set(&strings[1]);
						}

						message += strings[1];
					}
					else
					{
						if (variable->IsFloat())
						{
							message += std::to_string(*static_cast<const float*>(variable->Get()));
						}
						else if (variable->IsInteger())
						{
							message += std::to_string(*static_cast<const int32_t*>(variable->Get()));
						}
						else if (variable->IsString())
						{
							message += *static_cast<const std::string*>(variable->Get());
						}
					}

					VT_LOG(Trace, message);
				}
				else
				{
					VT_LOG(Trace, "Command {0} not found!", strings[0]);
				}
			}

			query.clear();
		}

		ImGui::PopStyleVar();
		ImGui::PopItemWidth();
	}
	ImGui::EndChild();
}

void LogPanel::SetupLogCategories()
{
	const Vector<LogCategoryBase*>& logCategories = LogCategoryRegistry::Get().GetRegisteredLogCategories();

	for (LogCategoryBase* category : logCategories)
	{
		auto& newCategory = m_logCategories.emplace_back();
		newCategory.category = category;
		newCategory.isActive = true;
	}
}

void LogPanel::TryAppendLogMessage(const LogCallbackData& logData)
{
	if (DoesLogEntryPassFilters(logData))
	{
		m_filteredLogMessages.emplace_back(logData);
	}
}

bool LogPanel::DoesLogEntryPassFilters(const LogCallbackData& logData)
{
	if (!m_logVerbosityActive[static_cast<size_t>(logData.severity)])
	{
		return false;
	}

	bool categoryActive = false;
	bool categoryFound = false;

	for (const LogCategoryData& category : m_logCategories)
	{
		if (logData.category == category.category)
		{
			if (category.isActive)
			{
				categoryActive = true;
			}

			categoryFound = true;
			break;
		}
	}

	// If the category is not found (was not registered for some reason, we add it)
	if (!categoryFound)
	{
		auto& newCategory = m_logCategories.emplace_back();
		newCategory.category = logData.category;
		newCategory.isActive = true;

		categoryActive = true;
	}

	if (!categoryActive)
	{
		return false;
	}

	if (!m_logSearchQuery.empty())
	{
		if (!Utility::StringContains(logData.message, m_logSearchQuery))
		{
			return false;
		}
	}

	return true;
}

void LogPanel::RefilterLogMessages()
{
	m_filteredLogMessages.clear();
	for (const LogCallbackData& logMessage : m_logMessages)
	{
		if (DoesLogEntryPassFilters(logMessage))
		{
			m_filteredLogMessages.emplace_back(logMessage);
		}
	}
}
