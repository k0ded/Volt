#include "vtapppch.h"
#include "BaseApplication.h"

#include <FileSystemModule/Filesystem.h>

#include <PlatformsModule/Platform.h>
#include <CoreModule/GlobalCommandLine.h>

namespace Volt
{
	BaseApplication* BaseApplication::s_instance = nullptr;

	Volt::BaseApplication::BaseApplication(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& appCreateInfo)
		: m_appCreateInfo(appCreateInfo), m_commandLineBuilder(commandLineBuilder)
	{
		VT_ASSERT_MSG(!s_instance, "Application already exists!");
		s_instance = this;

		// Enable / Disable logging.
		Log::Get().EnableLogging(IsLoggingEnabled());

		GlobalCommandLine::Initialize(commandLineBuilder);
		PlatformThread::Initialize();

		// Setup working directory.
		{
			Filesystem::Path workingDir;
			if (commandLineBuilder.IsArgDefined("workingdir"))
			{
				workingDir = commandLineBuilder.GetArgValue("workingdir");
			}

			Filesystem::InitializeWorkingDirectory(IsRuntime(), workingDir, commandLineBuilder.GetExecutableFilepath());
		}
	}

	BaseApplication::~BaseApplication()
	{
		PlatformThread::Shutdown();

		s_instance = nullptr;
	}
}
