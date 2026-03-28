#include "circuitpch.h"

#include <CoreModule/Console/ConsoleVariableRegistry.h>

namespace Circuit
{
	Volt::ConsoleVariable<int32_t> s_cvarCircuitShowWidgetBounds(
		"Circuit.Debug.ShowWidgetBounds",
		0,
		"Whether to show the bounds of all widgets.");

	Volt::ConsoleVariable<int32_t> s_cvarCircuitShowHoveredWidgetBounds(
		"Circuit.Debug.ShowHoveredWidgetBounds",
		0,
		"Whether to show the bounds of the hovered widget.");
}
