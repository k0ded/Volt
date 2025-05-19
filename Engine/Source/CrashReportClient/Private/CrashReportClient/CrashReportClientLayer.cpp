#include "CrashReportClient/CrashReportClientLayer.h"

#include <Volt-Platforms/Platform.h>

#include <Volt/Core/Application.h>

#include <imgui.h>
#include <imgui_stdlib.h>

namespace Volt
{
	void CrashReportClientLayer::OnAttach()
	{
		RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(CrashReportClientLayer::OnUpdateEvent));
		RegisterListener<Volt::AppImGuiUpdateEvent>(VT_BIND_EVENT_FN(CrashReportClientLayer::OnImGuiUpdateEvent));

		const CommandLineBuilder& commandLineBuilder = Application::Get().GetCommandLineBuilder();

		if (commandLineBuilder.IsArgDefined("monitorprocess"))
		{
			uint32_t processId = std::stoi(commandLineBuilder.GetArgValue("monitorprocess"));
			m_monitoredProcessHandle = PlatformProcess::OpenProcRestricted(processId);
		}

		if (commandLineBuilder.IsArgDefined("readpipe"))
		{
			m_monitoredReadPipe = reinterpret_cast<void*>(std::stoull(commandLineBuilder.GetArgValue("readpipe")));
		}

		if (commandLineBuilder.IsArgDefined("writepipe"))
		{
			m_monitoredWritePipe = reinterpret_cast<void*>(std::stoull(commandLineBuilder.GetArgValue("writepipe")));
		}

		m_crashContext = CreateScope<CrashContext>();
	}

	void CrashReportClientLayer::OnDetach()
	{
		PlatformProcess::CloseProc(m_monitoredProcessHandle);
	}

	bool CrashReportClientLayer::OnUpdateEvent(Volt::AppUpdateEvent& e)
	{
		if ((!m_monitoredProcessHandle.IsValid() || !PlatformProcess::IsProcRunning(m_monitoredProcessHandle)) && !m_isDisplayingCrash)
		{
			Application::Get().Quit();
		}

		if (!m_isDisplayingCrash)
		{
			const float appTargetDeltaTime = 1.f / 10.f;
			const float currentDeltaTime = Application::Get().GetFrameTimer().GetDeltaTime();
			const float timeToSleep = std::clamp(appTargetDeltaTime - currentDeltaTime, 0.f, 1.f);
			PlatformThread::Sleep<Time::Seconds>(timeToSleep);
		}

		HasMonitoredProcessCrashed();

		return false;
	}

	bool CrashReportClientLayer::OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e)
	{
		auto& io = ImGui::GetIO();

		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ io.DisplaySize.x, io.DisplaySize.y });

		if (ImGui::Begin("Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::Text("Volt has unfortunately crashed!");

			ImGui::Separator();

			ImGui::Text("Message");

			const ImVec2 messageSize = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.3f };
			ImGui::InputTextMultilineString("##Message", &m_crashMessage, messageSize);

			ImGui::Text("Stack Trace");

			const ImVec2 stackTraceSize = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 50.f };
			ImGui::InputTextMultiline("##StackTrace", m_crashContext->stackTrace, m_crashContext->stackTraceSize, stackTraceSize, ImGuiInputTextFlags_ReadOnly);

			if (ImGui::Button("Close without sending"))
			{
				Application::Get().Quit();
			}

			ImGui::SameLine();

			if (ImGui::Button("Send and close"))
			{
				Application::Get().Quit();
			}

			ImGui::End();
		}

		return false;
	}

	bool CrashReportClientLayer::HasMonitoredProcessCrashed()
	{
		Vector<uint8_t> data;
		if (PlatformProcess::ReadPipe(m_monitoredReadPipe, data))
		{
			if (data.size() == sizeof(CrashContext))
			{
				memcpy_s(m_crashContext.get(), sizeof(CrashContext), data.data(), data.size());
			}

			Application::Get().LaunchMainWindow();
			m_isDisplayingCrash = true;
			return true;
		}

		return false;
	}
}
