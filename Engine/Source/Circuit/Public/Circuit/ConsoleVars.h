#pragma once

#include <glm/fwd.hpp>
#include <CoreUtilities/ConsoleVariableRegistry.h>

namespace Circuit
{
	extern Volt::ConsoleVariable<int32_t> s_cvarCircuitShowWidgetBounds;
	extern Volt::ConsoleVariable<int32_t> s_cvarCircuitShowHoveredWidgetBounds;
	extern Volt::ConsoleVariable<int32_t> s_cvarCircuitLogPaint;
}
