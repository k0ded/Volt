#include <Volt-Platforms/Platform.h>
#include <Volt/Core/Application.h>

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/CommandLineBuilder.h>

#include <cstdint>
#include <iostream>

// Needs to be defined in the application creating code.
extern bool g_useCrashHandling;
extern Volt::Application* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder);

namespace Volt
{
	static bool ShouldHandleApplicationCrashes()
	{
		return !PlatformMisc::IsDebuggerPresent();
	}

	CrashReportingThread g_crashReportingThread(g_useCrashHandling && ShouldHandleApplicationCrashes());

	void ReportApplicationCrash(LPEXCEPTION_POINTERS exceptionInfo, const Volt::CommandLineBuilder& commandLineBuilder)
	{
		g_crashReportingThread.NotifyCrash(exceptionInfo, commandLineBuilder);
	}

	int32_t Main(const CommandLineBuilder& commandLineBuilder)
	{
		Application* app = CreateApplication(commandLineBuilder);
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
