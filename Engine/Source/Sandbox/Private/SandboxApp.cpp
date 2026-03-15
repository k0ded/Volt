#include "sbpch.h"

#include "Sandbox/Sandbox.h"

#include <Volt-Core/Config.h>
#include <Volt-Core/Project/ProjectManager.h>

#include <Volt-Application/Application.h>

#include <Volt-Platforms/Platform.h>
#include <Volt-FileSystem/FileUtility.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/JSON/JSONReader.h>

std::filesystem::path GetProjectPath(const Volt::CommandLineBuilder& commandLineBuilder)
{
	std::filesystem::path projectFilepath;
	if (commandLineBuilder.IsArgDefined("project"))
	{
		projectFilepath = commandLineBuilder.GetArgValue("project");
	}

	//try to find the project same way as the ProjectManager
	if (projectFilepath.empty())
	{
		for (const auto& dir : std::filesystem::directory_iterator("./"))
		{
			if (dir.path().extension() == ".vtproj")
			{
				projectFilepath = dir.path();
				break;
			}
		}
	}

	if (projectFilepath.empty())
	{
		VT_ASSERT_MSG(projectFilepath.empty(), "No project filepath provided!");
	}

	return projectFilepath;
}

bool PeekProjectVersionIsDeprecated(const std::filesystem::path& projectPath)
{
	std::string jsonString;

	std::ifstream stream(projectPath, std::ios::in | std::ios::binary | std::ios::ate);
	if (!stream.is_open())
	{
		const std::string error = std::format("Failed to open file: {0}!", projectPath.string());
		throw std::runtime_error(error.c_str());
		return false;
	}

	jsonString.resize(stream.tellg());
	stream.seekg(0);
	stream.read(jsonString.data(), jsonString.size());

	JSONReader jsonReader;
	if (!jsonReader.Parse(jsonString))
	{
		const std::string error = std::format("Project file {0} is invalid!", projectPath.string());
		throw std::runtime_error(error.c_str());
		return false;
	}

	std::string engineVersionStr;
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

void LaunchProjectUpgradeClient(const std::filesystem::path& projectPath)
{
	// As we at this point might be inside the binaries directory, we must also check if the crash reporter lies in the current directory.
	auto projectUpgradeClientFilepath = std::filesystem::current_path() / "Binaries\\ProjectUpgradeClient.exe";
	if (!FileSystem::Exists(projectUpgradeClientFilepath))
	{
		projectUpgradeClientFilepath = std::filesystem::current_path() / "ProjectUpgradeClient.exe";
	}

	if (!FileSystem::Exists(projectUpgradeClientFilepath))
	{
		throw std::runtime_error(std::format("Could not find the project upgrade client at '{0}'", projectUpgradeClientFilepath.string()));
		return;
	}

	Volt::CommandLineBuilder commandLineBuilder;
	commandLineBuilder.AddArgument("project", projectPath.string());
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
		std::filesystem::path projectPath = GetProjectPath(commandLineBuilder);

		if (projectPath.empty())
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
