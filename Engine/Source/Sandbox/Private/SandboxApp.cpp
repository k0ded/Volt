#include "sbpch.h"

#include "Sandbox/Sandbox.h"
#include "ProjectUpgrade/ProjectUpgradeLayer.h"

#include <Volt-Core/Project/ProjectManager.h>

#include <Volt-Application/Application.h>

class SandboxApp : public Volt::Application
{
public:
	SandboxApp( const Volt::CommandLineBuilder& commandLineBuilder, const Volt::ApplicationCreationInfo& appInfo)
		: Volt::Application(commandLineBuilder, appInfo)
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

Volt::BaseApplication* CreateApplicationBase(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Volt::ApplicationCreationInfo info{};
	info.iconPath = "Editor/Textures/Icons/icon_volt.dds";
	info.useVSync = true;
	info.enableImGui = true;
	info.useTitlebar = true;
	info.useCustomTitlebar = true;
	info.width = 1600;
	info.height = 900;

	return new SandboxApp(commandLineBuilder, info);
}
