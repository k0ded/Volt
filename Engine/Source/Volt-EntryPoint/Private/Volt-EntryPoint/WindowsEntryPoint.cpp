#include <PlatformsModule/Platform.h>
#include <Volt-Application/BaseApplication.h>

#include <Volt-Application/Application.h>
#include <CoreModule/ConfigManager.h>

#include <CoreModule/CommandLineBuilder.h>

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/Malloc.h>

#include <SubSystem/SubSystemManager.h>

#include <cstdint>
#include <iostream>

// Needs to be defined in the application creating code.
extern bool g_useCrashHandling;
extern Volt::BaseApplication* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder);

namespace Volt
{
	static bool ShouldHandleApplicationCrashes()
	{
		return !PlatformMisc::IsDebuggerPresent();
	}

	CrashReportingThread g_crashReportingThread(g_useCrashHandling && ShouldHandleApplicationCrashes());

	void ReportApplicationCrash(LPEXCEPTION_POINTERS exceptionInfo, const Volt::CommandLineBuilder& commandLineBuilder)
	{
		CrashReporterConnectionInfo connectionInfo;
		ConfigManager* configManager = SubSystemManager::GetSubSystem<ConfigManager>();
		if (configManager)
		{
			if (const ConfigValue* url = configManager->TryGetConfigValue("CrashReporter", "ServerURL"); url != nullptr)
			{
				connectionInfo.serverURL = url->Get<String>();
			}

			if (const ConfigValue* username = configManager->TryGetConfigValue("CrashReporter", "ServerUser"); username != nullptr)
			{
				connectionInfo.serverUser = username->Get<String>();
			}

			if (const ConfigValue* password = configManager->TryGetConfigValue("CrashReporter", "ServerPassword"); password != nullptr)
			{
				connectionInfo.serverPassword = password->Get<String>();
			}
		}

		g_crashReportingThread.NotifyCrash(exceptionInfo, commandLineBuilder.GetAsString(), connectionInfo);
	}

	int32_t Main(const CommandLineBuilder& commandLineBuilder)
	{
		Memory::Initialize();

		BaseApplication* app = CreateApplication(commandLineBuilder);
		if (!app)
		{
			return 1;
		}
		app->Run();

		delete app;

		return 0;
	}

	int32_t MainWrapper(const CommandLineBuilder& commandLineBuilder)
	{
		int32_t result = 0;

		if (ShouldHandleApplicationCrashes())
		{
			__try
			{
				result = Main(commandLineBuilder);
			}
			__except (ReportApplicationCrash(GetExceptionInformation(), commandLineBuilder), EXCEPTION_CONTINUE_SEARCH)
			{
			}
		}
		else
		{
			result = Main(commandLineBuilder);
		}


		return result;
	}

	int32_t LaunchApplication(HINSTANCE hInstance, HINSTANCE prevHInstance, PSTR cmdLine, int cmdShow)
	{
		// Build the command line
		int nArgs = 0;
		LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

		CommandLineBuilder commandLineBuilder;
		commandLineBuilder.BuildFromArgV(szArglist, nArgs);

		if (szArglist)
		{
			::LocalFree(szArglist);
		}

		int32_t result = 0;

		PlatformMisc::SetupExceptionHandlers();
		PlatformThread::SetupThreadConfig(false, false, true);

		if (ShouldHandleApplicationCrashes())
		{
			__try
			{
				result = MainWrapper(commandLineBuilder);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				PlatformMisc::RequestApplicationExit(true, 1);
			}

		}
		else
		{
			result = MainWrapper(commandLineBuilder);
		}

		return result;
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE prevHInstance, PSTR cmdLine, int cmdShow)
{
	return Volt::LaunchApplication(hInstance, prevHInstance, cmdLine, cmdShow);
}
