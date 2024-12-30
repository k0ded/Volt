#pragma once

#include <EntitySystem/ComponentReflection.h>
#include <EntitySystem/ComponentRegistry.h>

#include <PhysicsInterface/PhysicsTypes.h>

namespace Volt
{
	struct CharacterControllerComponent
	{
		PhysicsControllerActorNonWalkableMode climbingMode = PhysicsControllerActorNonWalkableMode::PreventClimbingAndForceSlide;

		float slopeLimit = 20.f;
		float invisibleWallHeight = 200.f;
		float maxJumpHeight = 100.f;
		float contactOffset = 1.f;
		float stepOffset = 10.f;
		float density = 1.f;
		uint32_t layer = 0;
		bool hasGravity = true;

		PhysicsActorID actorId;

		inline CharacterControllerComponent() = default;

		inline CharacterControllerComponent(PhysicsControllerActorNonWalkableMode aClimbingMode, float aSlopeLimit, float aInvisibleWallHeight, float aMaxJumpHeight,
			float aContactOffset, float aStepOffset, float aDensity, uint32_t aLayer, bool aHasGravity)
			: climbingMode(aClimbingMode), slopeLimit(aSlopeLimit), invisibleWallHeight(aInvisibleWallHeight), maxJumpHeight(aMaxJumpHeight), contactOffset(aContactOffset),
			stepOffset(aStepOffset), density(aDensity), layer(aLayer), hasGravity(aHasGravity)
		{
			layer = aLayer;
		}

		static void ReflectType(TypeDesc<CharacterControllerComponent>& reflect)
		{
			reflect.SetGUID("{DC5C002A-B72E-42A0-83FC-FFBE1FB2DEF2}"_guid);
			reflect.SetLabel("Character Controller Component");
			reflect.AddMember(&CharacterControllerComponent::climbingMode, "climbingMode", "Climbing Mode", "", PhysicsControllerActorNonWalkableMode::PreventClimbingAndForceSlide);
			reflect.AddMember(&CharacterControllerComponent::slopeLimit, "slopeLimit", "Slope Limit", "", 20.f);
			reflect.AddMember(&CharacterControllerComponent::invisibleWallHeight, "invisibleWallHeight", "Invisible Wall Height", "", 200.f);
			reflect.AddMember(&CharacterControllerComponent::maxJumpHeight, "maxJumpHeight", "Max Jump Height", "", 100.f);
			reflect.AddMember(&CharacterControllerComponent::contactOffset, "contactOffset", "Contact Offset", "", 1.f);
			reflect.AddMember(&CharacterControllerComponent::stepOffset, "stepOffset", "Step Offset", "", 10.f);
			reflect.AddMember(&CharacterControllerComponent::density, "density", "Density", "", 1.f);
			reflect.AddMember(&CharacterControllerComponent::layer, "layer", "Layer", "", 0);
			reflect.AddMember(&CharacterControllerComponent::hasGravity, "hasGravity", "Has Gravity", "", true);
		}

		REGISTER_COMPONENT(CharacterControllerComponent);
	};
}
