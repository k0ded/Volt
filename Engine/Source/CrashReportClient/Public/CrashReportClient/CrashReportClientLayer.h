#pragma once

#include <Volt-Application/ApplicationLayer.h>

#include <PlatformsModule/ProcessHandle.h>
#include <PlatformsModule/CrashContext.h>

#include <EventSystem/EventListener.h>
#include <EventSystem/ApplicationEvents.h>

namespace Volt
{
	class CrashReportClientLayer : public Volt::ApplicationLayer, public Volt::EventListener
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
		String m_crashMessage;

		CrashContext m_crashContext;
	};
}
