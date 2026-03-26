#pragma once

#include "Volt-Core/Config.h"

#include <CoreModule/CommandLineBuilder.h>

namespace GlobalCommandLine
{
	VTCORE_API void Initialize(const ::Volt::CommandLineBuilder& commandLineBuilder);
	VTCORE_API const ::Volt::CommandLineBuilder& Get();
}
