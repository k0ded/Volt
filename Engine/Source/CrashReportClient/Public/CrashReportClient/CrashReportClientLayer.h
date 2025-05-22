#pragma once

#include <Volt-Core/Layer/Layer.h>
#include <Volt-Platforms/ProcessHandle.h>

#include <EventSystem/EventListener.h>
#include <EventSystem/ApplicationEvents.h>

namespace Volt
{
	class CrashContext;
	class CrashReportClientLayer : public Volt::Layer, public Volt::EventListener
	{
	public:
		CrashReportClientLayer() = default;
		~CrashReportClientLayer() override = default;

		void OnAttach() override;
		void OnDetach() override;

	private:
		bool OnUpdateEvent(Volt::AppUpdateEvent& e);
		bool OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e);
		bool HasMonitoredProcessCrashed();

		void SendCrashReport();
		void RestartEngineAfterCrash();

		ProcessHandle m_monitoredProcessHandle;
		void* m_monitoredReadPipe = nullptr;
		void* m_monitoredWritePipe = nullptr;

		bool m_isDisplayingCrash = false;
		std::string m_crashMessage;

		std::string m_connectionURL;
		std::string m_connectionUsername;
		std::string m_connectionPassword;

		Scope<CrashContext> m_crashContext;
	};
}
