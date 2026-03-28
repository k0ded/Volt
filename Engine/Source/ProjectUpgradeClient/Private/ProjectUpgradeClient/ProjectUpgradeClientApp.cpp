#include "ProjectUpgradeClient/ProjectUpgradeClientLayer.h"

#include <PlatformsModule/Platform.h>

#include <Volt-Application/UIApplication.h>

class ProjectUpgradeClientApp : public Volt::UIApplication
{
public:
	ProjectUpgradeClientApp(const Volt::ApplicationCreationInfo& appInfo, const Volt::CommandLineBuilder& commandLineBuilder)
		: Volt::UIApplication(commandLineBuilder, appInfo)
	{ 
		if (commandLineBuilder.IsArgDefined("waitfordebugger"))
		{
			while (!Volt::PlatformMisc::IsDebuggerPresent()) {}
		}

		Volt::ProjectUpgradeClientLayer* projectUpgradeClientLayer = new Volt::ProjectUpgradeClientLayer();
		PushLayer(projectUpgradeClientLayer);
	}
};

bool g_useCrashHandling = true;

Volt::BaseApplication* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Volt::ApplicationCreationInfo appInfo{ };
	appInfo.title = "ProjectUpgradeClient";
	appInfo.width = 512;
	appInfo.height = 512;
	appInfo.createMainWindow = true;
	appInfo.enableImGuiViewports = false;
	appInfo.enableLogging = false;

	return new ProjectUpgradeClientApp(appInfo,commandLineBuilder);
}
