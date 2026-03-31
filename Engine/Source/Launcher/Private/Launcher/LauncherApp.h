#pragma once

#include <Volt-Application/Application_New.h>

#include <WindowModule/WindowHandle.h>

class LauncherApp : public Volt::Application_New
{
public:
	LauncherApp(const Volt::ApplicationCreationInfo& appInfo, const Volt::CommandLineBuilder& commandLineBuilder);

private:
	Volt::WindowHandle m_window;
};
