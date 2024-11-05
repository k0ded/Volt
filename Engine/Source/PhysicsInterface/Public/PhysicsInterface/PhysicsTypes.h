#pragma once

#include <CoreUtilities/Core.h>
#include <CoreUtilities/UUID.h>

#include <cstdint>

namespace Volt
{
	using PhysicsActorID = UUID64;
	using PhysicsColliderID = UUID64;

	enum class BroadphaseType : uint8_t
	{
		SweepAndPrune = 0,
		MultiBoxPrune,
		AutomaticBoxPrune
	};

	enum class FrictionType : uint8_t
	{
		Patch = 0,
		OneDirectional,
		TwoDirectional
	};

	enum class DebugType : uint8_t
	{
		None = 0,
		DebugToFile,
		LiveDebug
	};

	enum class ForceMode : uint8_t
	{
		Force = 0,
		Impulse,
		VelocityChange,
		Acceleration
	};

	enum class PhysicsActorLockFlags : uint8_t
	{
		None = 0,
		TranslationX = BIT(0), 
		TranslationY = BIT(1), 
		TranslationZ = BIT(2), 
		RotationX = BIT(3), 
		RotationY = BIT(4), 
		RotationZ = BIT(5), 
		Translation = TranslationX | TranslationY | TranslationZ,
		Rotation = RotationX | RotationY | RotationZ
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(PhysicsActorLockFlags)

	enum class PhysicsBodyType : uint8_t
	{
		Static = 0,
		Dynamic
	};

	enum class CollisionDetectionType : uint8_t
	{
		Discrete = 0,
		Continuous,
		ContinuousSpeculative
	};

	enum class PhysicsControllerActorNonWalkableMode : uint8_t
	{
		PreventClimbing,
		PreventClimbingAndForceSlide
	};

	enum class ColliderType : uint8_t
	{
		Box = 0,
		Sphere,
		Capsule,
		ConvexMesh,
		TriangleMesh
	};
}
