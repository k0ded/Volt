#include "Launcher/GameLayer.h"

#include "Testing/RenderingTestingLayer.h"

#include <Volt/Core/Application.h>

class LauncherApp : public Volt::Application
{
public:
	LauncherApp(const Volt::ApplicationInfo& appInfo, const Volt::CommandLineBuilder& commandLineBuilder)
		: Volt::Application(appInfo, commandLineBuilder)
	{
		//GameLayer* testing = new GameLayer();
		//PushLayer(testing);
	
		RenderingTestingLayer* testingLayer = new RenderingTestingLayer();
		PushLayer(testingLayer);
	}
private:
};

bool g_useCrashHandling = true;
Volt::Application* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Volt::ApplicationInfo info{};
	info.iconPath = "Editor/Textures/Icons/icon_volt.dds";
	info.useVSync = false;
	info.enableSteam = false;
	info.enableImGui = false;
	info.isRuntime = true;
	info.width = 1600;
	info.height = 900;

	return new LauncherApp(info, commandLineBuilder);
}
