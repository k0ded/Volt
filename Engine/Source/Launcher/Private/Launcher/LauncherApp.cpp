#include "Launcher/GameLayer.h"
#include "Launcher/TestingLayer.h"

#include <Volt-Application/Application.h>

class LauncherApp : public Volt::Application
{
public:
	LauncherApp(const Volt::ApplicationCreationInfo& appInfo, const Volt::CommandLineBuilder& commandLineBuilder)
		: Volt::Application(commandLineBuilder, appInfo)
	{
		TestingLayer* testing = new TestingLayer();
		PushLayer(testing);
	}
private:
};

bool g_useCrashHandling = true;
Volt::BaseApplication* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Volt::ApplicationCreationInfo info{};
	info.iconPath = "Editor/Textures/Icons/icon_volt.dds";
	info.useVSync = false;
	info.enableImGui = true;
	info.isRuntime = true;
	info.width = 1600;
	info.height = 900;

	return new LauncherApp(info, commandLineBuilder);
}
