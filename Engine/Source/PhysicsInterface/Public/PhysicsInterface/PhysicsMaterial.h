#pragma once

#include "PhysicsInterface/PhysicsHandleType.h"

namespace Volt
{
	struct PhysicsMaterialCreateInfo
	{
		float staticFriction = 0.1f;
		float dynamicFriction = 0.1f;
		float bounciness = 1.f;
	};

	class PhysicsMaterial : public PhysicsHandleType
	{
	public:
		virtual ~PhysicsMaterial() = default;

		virtual void SetStaticFriction(float friction) = 0;
		virtual void SetDynamicFriction(float friction) = 0;
		virtual void SetBounciness(float bounciness) = 0;

		virtual float GetStaticFriction() const = 0;
		virtual float GetDynamicFriction() const = 0;
		virtual float GetBounciness() const = 0;
	};
}
