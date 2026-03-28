#include "sbpch.h"

#include "Sandbox/Sandbox.h"

#include <CoreModule/Config.h>
#include <CoreModule/Project/ProjectManager.h>

#include <Volt-Application/Application.h>

#include <PlatformsModule/Platform.h>
#include <FileSystemModule/FileUtility.h>

#include <FileSystemModule/Filesystem.h>
#include <FileSystemModule/Iterators/DirectoryIterator.h>

#include <CoreModule/JSON/JSONReader.h>

Filesystem::Path GetProjectPath(const Volt::CommandLineBuilder& commandLineBuilder)
{
	Filesystem::Path projectFilepath;
	if (commandLineBuilder.IsArgDefined("project"))
	{
		projectFilepath = commandLineBuilder.GetArgValue("project");
	}

	//try to find the project same way as the ProjectManager
	if (projectFilepath.IsEmpty())
	{
		for (const auto& dir : Filesystem::DirectoryIterator("./"))
		{
			if (dir.path.Extension() == L".vtproj")
			{
				projectFilepath = dir.path;
				break;
			}
		}
	}

	if (projectFilepath.IsEmpty())
	{
		VT_ASSERT_MSG(projectFilepath.IsEmpty(), "No project filepath provided!");
	}

	return projectFilepath;
}

bool PeekProjectVersionIsDeprecated(const Filesystem::Path& projectPath)
{
	String jsonString;

	std::ifstream stream(projectPath.ToString().c_str(), std::ios::in | std::ios::binary | std::ios::ate);
	if (!stream.is_open())
	{
		const String error = FormatString("Failed to open file: {0}!", projectPath);
		throw std::runtime_error(error.c_str());
		return false;
	}

	jsonString.resize(stream.tellg());
	stream.seekg(0);
	stream.read(jsonString.data(), jsonString.size());

	JSONReader jsonReader;
	if (!jsonReader.Parse(jsonString))
	{
		const String error = FormatString("Project file {0} is invalid!", projectPath);
		throw std::runtime_error(error.c_str());
		return false;
	}

	String engineVersionStr;
	Volt::Version projectVersion;

	if (jsonReader.TryGet("EngineVersion", engineVersionStr))
	{
		projectVersion = engineVersionStr;
	}

	if (projectVersion != Volt::VT_VERSION)
	{
		//is deprecated
		return true;
	}

	return false;
}

void LaunchProjectUpgradeClient(const Filesystem::Path& projectPath)
{
	// As we at this point might be inside the binaries directory, we must also check if the crash reporter lies in the current directory.
	auto projectUpgradeClientFilepath = Filesystem::GetWorkingDirectory() / "Binaries\\ProjectUpgradeClient.exe";
	if (!Filesystem::Exists(projectUpgradeClientFilepath))
	{
		projectUpgradeClientFilepath = Filesystem::GetWorkingDirectory() / "ProjectUpgradeClient.exe";
	}

	if (!Filesystem::Exists(projectUpgradeClientFilepath))
	{
		const String tempString = FormatString("Could not find the project upgrade client at '{0}'", projectUpgradeClientFilepath);
		throw std::runtime_error(tempString.c_str());
		return;
	}

	Volt::CommandLineBuilder commandLineBuilder;
	commandLineBuilder.AddArgument("project", projectPath.ToString());
	//commandLineBuilder.AddArgument("waitfordebugger");

	Volt::PlatformProcess::CreateProc(
		projectUpgradeClientFilepath,
		commandLineBuilder.GetAsString(),
		true, false, nullptr);
}

class SandboxApp : public Volt::Application
{
public:
	SandboxApp(const Volt::CommandLineBuilder& commandLineBuilder, const Volt::ApplicationCreationInfo& appInfo)
		: Volt::Application(commandLineBuilder, appInfo)
	{
		if (commandLineBuilder.IsArgDefined("waitfordebugger"))
		{
			while (!Volt::PlatformMisc::IsDebuggerPresent()) {}
		}

		Sandbox* sandbox = new Sandbox();
		PushLayer(sandbox);
	}
};

bool g_useCrashHandling = true;

Volt::BaseApplication* CreateApplication(const Volt::CommandLineBuilder& commandLineBuilder)
{
	//if the project is deprecated, dont load anything and instead launch the project upgrade client
	{
		Filesystem::Path projectPath = GetProjectPath(commandLineBuilder);

		if (projectPath.IsEmpty())
		{
			return nullptr;
		}

		if (PeekProjectVersionIsDeprecated(projectPath))
		{
			LaunchProjectUpgradeClient(projectPath);
			return nullptr;
		}
	}

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
