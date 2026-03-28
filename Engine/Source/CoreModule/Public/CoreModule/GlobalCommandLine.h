#pragma once

#include "CoreModule/Config.h"

#include <CoreModule/CommandLineBuilder.h>

namespace GlobalCommandLine
{
	VTC_API void Initialize(const ::Volt::CommandLineBuilder& commandLineBuilder);
	VTC_API const ::Volt::CommandLineBuilder& Get();
}
