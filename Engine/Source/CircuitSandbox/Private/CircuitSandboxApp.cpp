#include "csbpch.h"

#include "CircuitSandbox/CircuitSandbox.h"

#include <Volt-Application/Application.h>

#include <Volt-Platforms/Platform.h>

class CircuitSandboxApp : public Volt::Application
{
public:
	CircuitSandboxApp(const Volt::CommandLineBuilder& commandLineBuilder, const Volt::ApplicationCreationInfo& appInfo)
		: Volt::Application(commandLineBuilder, appInfo)
	{
		if (commandLineBuilder.IsArgDefined("waitfordebugger"))
		{
			while (!Volt::PlatformMisc::IsDebuggerPresent()) {}
		}

		CircuitSandbox* sandbox = new CircuitSandbox();
		PushLayer(sandbox);
	}
};

bool g_useCrashHandling = true;

Volt::BaseApplication* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Volt::ApplicationCreationInfo info{};
	info.title = "Circuit Sandbox";
	info.iconPath = "Editor/Textures/Icons/icon_volt.dds";
	info.useVSync = true;
	info.enableImGui = false;
	info.useTitlebar = true;
	info.useCustomTitlebar = false;
	info.width = 1600;
	info.height = 900;

	return new CircuitSandboxApp(commandLineBuilder, info);
}
