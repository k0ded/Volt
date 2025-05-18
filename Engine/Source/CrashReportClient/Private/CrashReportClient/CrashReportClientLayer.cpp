#include "CrashReportClient/CrashReportClientLayer.h"

#include <Volt-Platforms/Platform.h>

#include <Volt/Core/Application.h>

namespace Volt
{
	void CrashReportClientLayer::OnAttach()
	{
		RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(CrashReportClientLayer::OnUpdateEvent));

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

	bool CrashReportClientLayer::HasMonitoredProcessCrashed()
	{
		Vector<uint8_t> data;
		if (PlatformProcess::ReadPipe(m_monitoredReadPipe, data))
		{
			Application::Get().LaunchMainWindow();
			m_isDisplayingCrash = true;
			return true;
		}

		return false;
	}
}
