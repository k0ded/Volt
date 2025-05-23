#include "CrashReportClient/CrashReportClientLayer.h"

#include <Volt-Platforms/Platform.h>

#include <Volt/Core/Application.h>
#include <Volt/Utility/UIUtility.h>

#include <CoreUtilities/FileIO/YAMLFileStreamReader.h>

#include <nlohmann/json.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>

namespace Volt
{
	inline static glm::vec4 ToNormalizedRGB(float r, float g, float b, float a = 255.f)
	{
		return { r / 255.f, g / 255.f, b / 255.f, a / 255.f };
	}

	inline static const UI::Button BlueButton = { ToNormalizedRGB(0.f, 112.f, 224.f), ToNormalizedRGB(14.f, 134.f, 225.f), ToNormalizedRGB(0.f, 80.f, 160.f) };
	inline static const UI::Button DefaultButton = { ToNormalizedRGB(56.f, 56.f, 56.f), ToNormalizedRGB(87.f, 87.f, 87.f), ToNormalizedRGB(47.f, 47.f, 47.f) };

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

		YAMLFileStreamReader fileReader{};
		if (fileReader.OpenFile("Engine/EngineConfig.vtconfig"))
		{
			fileReader.EnterScope("EngineConfig");
			m_connectionURL = fileReader.ReadAtKey("crashReporterServerURL", std::string());
			m_connectionUsername = fileReader.ReadAtKey("crashReporterServerUsername", std::string());
			m_connectionPassword = fileReader.ReadAtKey("crashReporterServerPassword", std::string());
			fileReader.ExitScope();
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
			ImGui::InputTextMultiline("##StackTrace", m_crashContext->stackTrace, strlen(m_crashContext->stackTrace), stackTraceSize, ImGuiInputTextFlags_ReadOnly);

			{
				UI::ScopedButtonColor color{ DefaultButton };
				if (ImGui::Button("Close without sending"))
				{
					Application::Get().Quit();
				}
			}

			ImGui::SameLine();

			{
				UI::ScopedButtonColor color{ DefaultButton };
				if (ImGui::Button("Send and close"))
				{
					SendCrashReport();

					Application::Get().Quit();
				}
			}

			ImGui::SameLine();

			{
				UI::ScopedButtonColor color{ BlueButton };
				if (ImGui::Button("Send and restart"))
				{
					SendCrashReport();
					RestartEngineAfterCrash();

					Application::Get().Quit();
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

	void CrashReportClientLayer::SendCrashReport()
	{
		using json = nlohmann::json;

		// Our working directory is in the Engine directory.
		std::ifstream istream("Log/Log.txt");

		std::stringstream logStr;
		if (istream.is_open())
		{
			logStr << istream.rdbuf();
		}

		json j;
		j["user"] = std::string(m_crashContext->userName);
		j["timestamp"] = std::string(m_crashContext->timestamp);
		j["log"] = logStr.str();
		j["stackTrace"] = std::string(m_crashContext->stackTrace);
		j["message"] = m_crashMessage;
		j["error"] = std::string(m_crashContext->errorString);

		std::stringstream sstream;
		sstream << j;

		PlatformFTPClient ftpClient;

		FTPClientConnectInfo connectInfo;
		connectInfo.username = m_connectionUsername;
		connectInfo.password = m_connectionPassword;
		connectInfo.url = m_connectionURL;
		ftpClient.Connect(connectInfo);

		const std::string fileame = "VoltCrashLogs/CrashReport_" + std::string(m_crashContext->timestamp) + ".json";
		ftpClient.UploadStringAsFile(fileame, sstream.str());
	}

	void CrashReportClientLayer::RestartEngineAfterCrash()
	{
		// As we have inherited the working directory from the engine we need to enter the binaries directory.
		const auto sandboxFilepath = std::filesystem::current_path() / "Binaries\\Sandbox.exe";
		PlatformProcess::CreateProc(sandboxFilepath, std::string(m_crashContext->commandLine), true, false, nullptr);
	}
}
