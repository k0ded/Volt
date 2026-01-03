#include "vtcorepch.h"

#include "Volt-Core/GlobalCommandLine.h"

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
