#pragma once
#include <glm/vec4.hpp>

namespace UI
{
	struct ButtonColorInfo
	{
		glm::vec4 normal = { 1.f, 1.f, 1.f, 1.f };
		glm::vec4 hovered = { 1.f, 1.f, 1.f, 1.f };
		glm::vec4 active = { 1.f, 1.f, 1.f, 1.f };
	};
}
