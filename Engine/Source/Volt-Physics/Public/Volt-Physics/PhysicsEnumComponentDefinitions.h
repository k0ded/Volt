#pragma once

#include <PhysicsInterface/PhysicsTypes.h>

#include <EntitySystem/ComponentRegistry.h>

namespace Volt
{
	static void ReflectType(TypeDesc<CollisionDetectionType>& reflect)
	{
		reflect.SetGUID("{E3CCC646-C301-492D-AD4B-A983B7B2BE72}"_guid);
		reflect.SetLabel("Collision Detection");
		reflect.SetDefaultValue(CollisionDetectionType::Discrete);
		reflect.AddConstant(CollisionDetectionType::Discrete, "discrete", "Discrete");
		reflect.AddConstant(CollisionDetectionType::Continuous, "continuous", "Continuous");
		reflect.AddConstant(CollisionDetectionType::ContinuousSpeculative, "continuousSpeculative", "Continuous Speculative");
	}

	static void ReflectType(TypeDesc<PhysicsBodyType>& reflect)
	{
		reflect.SetGUID("{98C6CC44-5B1A-4E20-BFA1-9DEBD31BE418}"_guid);
		reflect.SetLabel("Body Type");
		reflect.SetDefaultValue(PhysicsBodyType::Static);
		reflect.AddConstant(PhysicsBodyType::Static, "static", "Static");
		reflect.AddConstant(PhysicsBodyType::Dynamic, "dynamic", "Dynamic");
	}

	static void ReflectType(TypeDesc<PhysicsControllerActorNonWalkableMode>& reflect)
	{
		reflect.SetGUID("{0871B88E-30A5-4082-B057-D088D4B1DD23}"_guid);
		reflect.SetLabel("Non Walkable Mode");
		reflect.SetDefaultValue(PhysicsControllerActorNonWalkableMode::PreventClimbing);
		reflect.AddConstant(PhysicsControllerActorNonWalkableMode::PreventClimbing, "preventClimbing", "Prevent Climbing");
		reflect.AddConstant(PhysicsControllerActorNonWalkableMode::PreventClimbingAndForceSlide, "preventClimbingAndForceSlide", "Prevent Climbing And Force Slide");
	}
}
