#include "cpch.h"
#include "CoreModule/GlobalCommandLine.h"

namespace GlobalCommandLine
{
	Volt::CommandLineBuilder g_globalCommandLineBuilder;

	void Initialize(const ::Volt::CommandLineBuilder& commandLineBuilder)
	{
		g_globalCommandLineBuilder = commandLineBuilder;
	}

	const Volt::CommandLineBuilder& Get()
	{
		return g_globalCommandLineBuilder;
	}
}
