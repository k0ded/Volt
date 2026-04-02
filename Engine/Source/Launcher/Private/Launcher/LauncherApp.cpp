#include "Launcher/GameLayer.h"
#include "Launcher/LauncherApp.h"

#include <WindowModule/WindowManager_New.h>
#include <WindowModule/Window_New.h>

bool g_useCrashHandling = true;
Volt::BaseApplication* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Volt::ApplicationCreationInfo info{};
	info.iconPath = "Editor/Textures/Icons/icon_volt.ico";
	info.useVSync = true;
	info.enableImGui = false;
	info.useTitlebar = true;
	info.useCustomTitlebar = true;
	info.isRuntime = false;
	info.width = 1600;
	info.height = 900;
	info.title = "Launcher";

	return new LauncherApp(info, commandLineBuilder);
}

LauncherApp::LauncherApp(const Volt::ApplicationCreationInfo& appInfo, const Volt::CommandLineBuilder& commandLineBuilder)
	: Volt::Application_New(commandLineBuilder, appInfo)
{
	Volt::WindowInitializer windowInitializer{};
	windowInitializer.title = WString(WString::CtorConvert(), appInfo.title);
	windowInitializer.iconFilepath = appInfo.iconPath;
	windowInitializer.initialWidth = appInfo.width;
	windowInitializer.initialHeight = appInfo.height;
	windowInitializer.initialPosX = 0;
	windowInitializer.initialPosY = 0;
	windowInitializer.enableVSync = appInfo.useVSync;

	m_window = Volt::WindowManager_New::Get().CreateWindow(windowInitializer);

	GameLayer* testing = new GameLayer(m_window);
	PushLayer(testing);
}
