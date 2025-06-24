#include "CrashReportClient/CrashReportClientLayer.h"

#include <Volt-Platforms/Platform.h>

#include <Volt-Application/UIApplication.h>

class CrashReportClientApp : public Volt::UIApplication
{
public:
	CrashReportClientApp(const Volt::ApplicationCreationInfo& appInfo, const Volt::CommandLineBuilder& commandLineBuilder)
		: Volt::UIApplication(commandLineBuilder, appInfo)
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

Volt::BaseApplication* CreateApplicationBase(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Volt::ApplicationCreationInfo appInfo{ };
	appInfo.title = "CrashReportClient";
	appInfo.width = 512;
	appInfo.height = 512;
	appInfo.createMainWindow = false;
	appInfo.enableImGuiViewports = false;
	appInfo.enableLogging = false;

	return new CrashReportClientApp(appInfo,commandLineBuilder);
}
