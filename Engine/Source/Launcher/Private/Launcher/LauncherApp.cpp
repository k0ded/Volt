#include "Launcher/GameLayer.h"

#include <Volt-Application/Application.h>

class LauncherApp : public Volt::Application
{
public:
	LauncherApp(const Volt::ApplicationCreationInfo& appInfo, const Volt::CommandLineBuilder& commandLineBuilder)
		: Volt::Application(commandLineBuilder, appInfo)
	{
		GameLayer* testing = new GameLayer();
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
	info.enableImGui = false;
	info.isRuntime = true;
	info.width = 1600;
	info.height = 900;

	return new LauncherApp(info, commandLineBuilder);
}
