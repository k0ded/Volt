#pragma once

#include <string>

namespace Volt
{
	using PhysicsLayerID = uint32_t;

	struct PhysicsLayer
	{
		PhysicsLayerID id = 0;
		uint32_t bit = 0;
		uint32_t collidesWithBitMask = 0;
		String name;

		inline bool IsValid() const
		{
			return id != 0;
		}
	};
}
