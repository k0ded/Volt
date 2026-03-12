#include "sbpch.h"

#include "Sandbox/NodeGraph/PinDrawerRegistry.h"

#include <NodeGraph/NodeGraphBase.h>

#include <imgui.h>

REGISTER_PIN_DRAWER_FOR_TYPE(PinType_Float)
{
	ImGuiSliderFlags flags = 0;
	if (customData.isSlider)
	{
		ImGui::SliderFloat("##Value", &storage.Value, customData.minBound, customData.maxBound, "%.3f", flags);
	}
	else
	{
		ImGui::DragFloat("##Value", &storage.Value, customData.speed, customData.minBound, customData.maxBound, "%.3f", flags);
	}
}
