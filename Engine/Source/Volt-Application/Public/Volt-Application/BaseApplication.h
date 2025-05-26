#pragma once

#include <Volt-Core/Version.h>

#include <CoreUtilities/CommandLineBuilder.h>

namespace Volt
{
	struct ApplicationCreationInfo
	{
		bool isRuntime = false;
		bool enableLogging = true;
		Version version = VT_VERSION;
	};

	class ApplicationLayer;

	class BaseApplication
	{
	public:
		BaseApplication(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& appCreateInfo = {})
			: m_appCreateInfo(appCreateInfo), m_commandLineBuilder(commandLineBuilder)
		{ }

		virtual ~BaseApplication() = default;
		virtual void Run() = 0;
		virtual void Quit() = 0;
		virtual void PushLayer(ApplicationLayer* layer) = 0;
		virtual void PopLayer(ApplicationLayer* layer) = 0;


	protected:
		ApplicationCreationInfo m_appCreateInfo;
		const CommandLineBuilder m_commandLineBuilder;
	};
}
