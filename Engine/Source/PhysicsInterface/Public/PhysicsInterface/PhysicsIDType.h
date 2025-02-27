#pragma once

#include "PhysicsInterface/PhysicsTypes.h"

namespace Volt
{
	class PhysicsIDType
	{
	public:
		virtual ~PhysicsIDType() = default;
		virtual PhysicsActorID GetID() const = 0;
	};
}
