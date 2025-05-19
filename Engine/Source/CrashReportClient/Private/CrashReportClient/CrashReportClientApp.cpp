#include "CrashReportClient/CrashReportClientLayer.h"

#include <Volt-Platforms/Platform.h>

#include <Volt/Core/Application.h>

class CrashReportClientApp : public Volt::Application
{
public:
	CrashReportClientApp(const Volt::ApplicationInfo& appInfo, const Volt::CommandLineBuilder& commandLineBuilder)
		: Volt::Application(appInfo, commandLineBuilder)
	{ 
		if (commandLineBuilder.IsArgDefined("waitfordebugger"))
		{
			while (!Volt::PlatformMisc::IsDebuggerPresent()) {}
		}

		Volt::CrashReportClientLayer* crashReportingClientLayer = new Volt::CrashReportClientLayer();
		PushLayer(crashReportingClientLayer);
	}
};

bool g_useCrashHandling = false;
Volt::Application* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Volt::ApplicationInfo appInfo{};
	appInfo.title = "CrashReportClient";
	appInfo.width = 512;
	appInfo.height = 512;
	appInfo.createMainWindow = false;
	appInfo.enableImGuiViewports = false;

	return new CrashReportClientApp(appInfo, commandLineBuilder);
}
