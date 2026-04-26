#include "circuitpch.h"

#include <CoreUtilities/ConsoleVariableRegistry.h>

namespace Circuit
{
	ConsoleVariable<int32_t> s_cvarCircuitShowWidgetBounds(
		"Circuit.Debug.ShowWidgetBounds",
		0,
		"Whether to show the bounds of all widgets.");

	ConsoleVariable<int32_t> s_cvarCircuitShowHoveredWidgetBounds(
		"Circuit.Debug.ShowHoveredWidgetBounds",
		0,
		"Whether to show the bounds of the hovered widget.");

	ConsoleVariable<int32_t> s_cvarCircuitLogPaint(
		"Circuit.Debug.LogPaint",
		0,
		"Whether to log OnPaint.");
}
