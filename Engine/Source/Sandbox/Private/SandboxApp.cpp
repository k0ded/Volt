#include "sbpch.h"

#include "Sandbox/Sandbox.h"
#include "ProjectUpgrade/ProjectUpgradeLayer.h"

#include <Volt/EntryPoint.h>

#include <Volt/Core/Application.h>
#include <Volt-Core/Project/ProjectManager.h>

class SandboxApp : public Volt::Application
{
public:
	SandboxApp(const Volt::ApplicationInfo& appInfo, const Volt::CommandLineBuilder& commandLineBuilder)
		: Volt::Application(appInfo, commandLineBuilder)
	{
		if (Volt::ProjectManager::GetProject().isDeprecated)
		{
			ProjectUpgradeLayer* layer = new ProjectUpgradeLayer();
			PushLayer(layer);
		}
		else
		{
			Sandbox* sandbox = new Sandbox();
			PushLayer(sandbox);
		}
	}
};

bool g_useCrashHandling = true;
Volt::Application* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Volt::ApplicationInfo info{};
	info.iconPath = "Editor/Textures/Icons/icon_volt.dds";
	info.useVSync = false;
	info.enableSteam = false;
	info.enableImGui = true;
	info.useTitlebar = true;
	info.useCustomTitlebar = true;
	info.width = 1600;
	info.height = 900;

	return new SandboxApp(info, commandLineBuilder);
}
