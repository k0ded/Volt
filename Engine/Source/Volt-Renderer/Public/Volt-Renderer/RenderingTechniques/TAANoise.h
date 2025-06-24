#pragma once

#include <glm/glm.hpp>

namespace Volt
{
	struct TAANoise
	{
		TAANoise();

		glm::vec2 Get(uint32_t frameIndex, const glm::uvec2& renderSize);

		inline static float s_haltonX[8];
		inline static float s_haltonY[8];
		inline static bool s_initialized = false;
	};
}
