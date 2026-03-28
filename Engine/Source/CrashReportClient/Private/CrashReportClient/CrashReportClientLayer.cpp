#include "CrashReportClient/CrashReportClientLayer.h"

#include <PlatformsModule/Platform.h>

#include <Volt-Application/BaseApplication.h>
#include <Volt-Application/UI/UIUtility.h>

#include <CoreModule/ConfigManager.h>
#include <FileSystemModule/FileUtility.h>
#include <FileSystemModule/Filesystem.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreModule/JSON/JSONWriter.h>

#include <CoreUtilities/Archive/MemoryArchive.h>

#include <fstream>
#include <imgui.h>
#include <imgui_stdlib.h>

namespace Volt
{
	inline static glm::vec4 ToNormalizedRGB(float r, float g, float b, float a = 255.f)
	{
		return { r / 255.f, g / 255.f, b / 255.f, a / 255.f };
	}

	inline static const UI::ButtonColorInfo BlueButton = { ToNormalizedRGB(0.f, 112.f, 224.f), ToNormalizedRGB(14.f, 134.f, 225.f), ToNormalizedRGB(0.f, 80.f, 160.f) };
	inline static const UI::ButtonColorInfo DefaultButton = { ToNormalizedRGB(56.f, 56.f, 56.f), ToNormalizedRGB(87.f, 87.f, 87.f), ToNormalizedRGB(47.f, 47.f, 47.f) };

	void CrashReportClientLayer::OnAttach()
	{
		RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(CrashReportClientLayer::OnUpdateEvent));
		RegisterListener<Volt::AppImGuiUpdateEvent>(VT_BIND_EVENT_FN(CrashReportClientLayer::OnImGuiUpdateEvent));

		const CommandLineBuilder& commandLineBuilder = BaseApplication::Get().GetCommandLineBuilder();

		if (commandLineBuilder.IsArgDefined("monitorprocess"))
		{
			uint32_t processId = StoI(commandLineBuilder.GetArgValue("monitorprocess"));
			m_monitoredProcessHandle = PlatformProcess::OpenProcRestricted(processId);
		}

		if (commandLineBuilder.IsArgDefined("readpipe"))
		{
			m_monitoredReadPipe = reinterpret_cast<void*>(StoUll(commandLineBuilder.GetArgValue("readpipe")));
		}

		if (commandLineBuilder.IsArgDefined("writepipe"))
		{
			m_monitoredWritePipe = reinterpret_cast<void*>(StoUll(commandLineBuilder.GetArgValue("writepipe")));
		}
	}

	void CrashReportClientLayer::OnDetach()
	{
		PlatformProcess::CloseProc(m_monitoredProcessHandle);
	}

	bool CrashReportClientLayer::OnUpdateEvent(Volt::AppUpdateEvent& e)
	{
		if ((!m_monitoredProcessHandle.IsValid() || !PlatformProcess::IsProcRunning(m_monitoredProcessHandle)) && !m_isDisplayingCrash)
		{
			BaseApplication::Get().Quit();
		}

		if (!m_isDisplayingCrash)
		{
			const float appTargetDeltaTime = 1.f / 10.f;
			const float currentDeltaTime = e.GetTimestep();
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
			ImGui::InputTextMultiline("##Message", &m_crashMessage, messageSize);

			ImGui::Text("Stack Trace");

			const ImVec2 stackTraceSize = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 50.f };
			ImGui::InputTextMultiline("##StackTrace", &m_crashContext.stackTrace, stackTraceSize, ImGuiInputTextFlags_ReadOnly);

			{
				UI::ScopedButtonColor color{ DefaultButton };
				if (ImGui::Button("Close without sending"))
				{
					BaseApplication::Get().Quit();
				}
			}

			ImGui::SameLine();

			{
				UI::ScopedButtonColor color{ DefaultButton };
				if (ImGui::Button("Send and close"))
				{
					SendCrashReport();

					BaseApplication::Get().Quit();
				}
			}

			ImGui::SameLine();

			{
				UI::ScopedButtonColor color{ BlueButton };
				if (ImGui::Button("Send and restart"))
				{
					SendCrashReport();
					RestartEngineAfterCrash();

					BaseApplication::Get().Quit();
				}
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
			MemoryReader memoryReader;
			memoryReader << m_crashContext;

			BaseApplication::Get().LaunchMainWindow();
			m_isDisplayingCrash = true;
			return true;
		}

		return false;
	}

	void CrashReportClientLayer::SendCrashReport()
	{
		const Filesystem::Path logFilepath = "Log/Log.txt";

		String logStr;
		FileUtility::ReadStringFromFile(logFilepath, logStr);

		JSONWriter jsonWriter;
		jsonWriter.AppendKeyValue("user", m_crashContext.username);
		jsonWriter.AppendKeyValue("timestamp", m_crashContext.timestamp);
		jsonWriter.AppendKeyValue("log", logStr);
		jsonWriter.AppendKeyValue("stackTrace", m_crashContext.stackTrace);
		jsonWriter.AppendKeyValue("message", m_crashMessage);
		jsonWriter.AppendKeyValue("error", m_crashContext.errorString);

		PlatformFTPClient ftpClient;

		FTPClientConnectInfo connectInfo;
		connectInfo.username = m_crashContext.serverUser;
		connectInfo.password = m_crashContext.serverPassword;
		connectInfo.url = m_crashContext.serverURL;
		ftpClient.Connect(connectInfo);

		const String fileame = "VoltCrashLogs/CrashReport_" + m_crashContext.timestamp + ".json";
		ftpClient.UploadStringAsFile(fileame, jsonWriter.View());
	}

	void CrashReportClientLayer::RestartEngineAfterCrash()
	{
		// As we have inherited the working directory from the engine we need to enter the binaries directory.
		const auto sandboxFilepath = Filesystem::GetWorkingDirectory() / "Binaries\\Sandbox.exe";
		PlatformProcess::CreateProc(sandboxFilepath, m_crashContext.commandLine, true, false, nullptr);
	}
}
