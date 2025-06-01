#include "vtapppch.h"
#include "BaseApplication.h"

namespace Volt
{
	BaseApplication* BaseApplication::s_instance = nullptr;

	Volt::BaseApplication::BaseApplication(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& appCreateInfo)
		: m_appCreateInfo(appCreateInfo), m_commandLineBuilder(commandLineBuilder)
	{
		s_instance = this;
	}
}
