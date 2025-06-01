#pragma once

#include "Volt-Application/Config.h"

#include <Volt-Core/Version.h>

#include <CoreUtilities/CommandLineBuilder.h>

#include <WindowModule/WindowMode.h>

#include <string>
#include <filesystem>

namespace Volt
{
	struct ApplicationCreationInfo
	{
		std::string title;

		std::filesystem::path iconPath;
		std::filesystem::path cursorPath;

		WindowMode windowMode = WindowMode::Windowed;

		uint32_t width = 1280;
		uint32_t height = 720;

		bool createMainWindow = true;

		bool useTitlebar = true;
		bool useCustomTitlebar = false;

		bool useVSync = true;
		bool isRuntime = false;
		bool enableLogging = true;

		bool enableImGui = true;
		bool enableImGuiViewports = true; 

		Version version = VT_VERSION;
	};

	class ApplicationLayer;

	class VTAPP_API BaseApplication
	{
	public:
		BaseApplication(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& appCreateInfo = {});

		virtual ~BaseApplication() = default;
		virtual void Run() = 0;
		virtual void Quit() = 0;
		virtual void PushLayer(ApplicationLayer* layer) = 0;
		virtual void PopLayer(ApplicationLayer* layer) = 0;
		virtual void LaunchMainWindow() = 0;

		const ApplicationCreationInfo& GetCreateInfo() { return m_appCreateInfo; }
		const CommandLineBuilder& GetCommandLineBuilder() const { return m_commandLineBuilder; }
	protected:
		ApplicationCreationInfo m_appCreateInfo;
		const CommandLineBuilder m_commandLineBuilder;

	public:
		inline static BaseApplication& Get() { return *s_instance; }
	private:
		static BaseApplication* s_instance;
	};
}
