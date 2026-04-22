#pragma once

#include <InputModule/InputCodes.h>
#include <glm/vec2.hpp>

namespace Circuit
{
	struct WidgetInteractionData
	{
		glm::vec2 mousePos = { 0, 0 };

		glm::vec2 mouseDragDelta = { 0, 0 };

		glm::vec2 scrollDelta = { 0, 0 };

		Volt::InputCode mouseButton = Volt::InputCode::Unknown;
	};
}
