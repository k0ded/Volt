#include "vtapppch.h"
#include "BaseApplication.h"

#include <Volt-Platforms/Platform.h>

namespace Volt
{
	BaseApplication* BaseApplication::s_instance = nullptr;

	Volt::BaseApplication::BaseApplication(const CommandLineBuilder& commandLineBuilder, const ApplicationCreationInfo& appCreateInfo)
		: m_appCreateInfo(appCreateInfo), m_commandLineBuilder(commandLineBuilder)
	{
		VT_ASSERT_MSG(!s_instance, "Application already exists!");
		s_instance = this;

		PlatformThread::Initialize();
	}
	BaseApplication::~BaseApplication()
	{
		s_instance = nullptr;
	}
}
