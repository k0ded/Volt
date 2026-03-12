#pragma once

#include <cstdint>

namespace Volt
{
	enum class MaterialBlendMode : uint8_t
	{
		Opaque = 0,
		AlphaMasked,
		Translucent,
		Count
	};
}
