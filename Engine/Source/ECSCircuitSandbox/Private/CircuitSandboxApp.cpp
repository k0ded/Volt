#include "ecscsbpch.h"

#include "CircuitSandboxApp.h"
#include "ECSCircuitSandbox/CircuitSandbox.h"

#include <PlatformsModule/Platform.h>

bool g_useCrashHandling = true;
Volt::BaseApplication* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Volt::ApplicationCreationInfo info{};
	info.title = "ECS Circuit Sandbox";
	info.iconPath = "Editor/Textures/Icons/icon_volt.ico";
	info.useVSync = true;
	info.enableImGui = false;
	info.useTitlebar = true;
	info.useCustomTitlebar = true;
	info.width = 1600;
	info.height = 900;

	return new CircuitSandboxApp(info, commandLineBuilder);
}

CircuitSandboxApp::CircuitSandboxApp(const Volt::ApplicationCreationInfo& appInfo, const Volt::CommandLineBuilder& commandLineBuilder)
	: Volt::Application_New(commandLineBuilder, appInfo)
{
	CircuitSandbox* sandbox = new CircuitSandbox();
	PushLayer(sandbox);
}
