#pragma once

#include "PhysicsInterface/PhysicsTypes.h"
#include "PhysicsInterface/PhysicsMaterial.h"
#include "PhysicsInterface/PhysicsHandleType.h"
#include "PhysicsInterface/PhysicsIDType.h"
#include "PhysicsInterface/PhysicsLayer.h"

#include <glm/glm.hpp>

namespace Volt
{
	struct PhysicsControllerActorCreateInfo
	{
		glm::vec3 initialPosition = 0.f;
		glm::vec3 upDirection = { 0.f, 1.f, 0.f };
		float slopeLimitDegrees = 20.f;
		float invisibleWallHeight = 200.f;
		float maxJumpHeight = 100.f;
		float contactOffset = 1.f;
		float stepOffset = 10.f;
		float density = 1.f;
		float radius = 50.f;
		float height = 100.f;

		uint32_t physicsLayer = 0;
		bool disableGravity = false;
		PhysicsControllerActorNonWalkableMode nonWalkableMode = PhysicsControllerActorNonWalkableMode::PreventClimbing;
		Ref<PhysicsMaterial> physicalMaterial;
	
		std::string debugName;
	};

	class PhysicsControllerActor : public PhysicsHandleType, public PhysicsIDType
	{
	public:
		virtual ~PhysicsControllerActor() {}

		virtual void Release() = 0;

		virtual void SetRadius(float radius) = 0;
		virtual void SetHeight(float height) = 0;
		virtual void SetPosition(const glm::vec3& position) = 0;
		virtual void SetFootPosition(const glm::vec3& footPosition) = 0;
		virtual void SetAngularVelocity(const glm::vec3& velocity) = 0;
		virtual void SetLinearVelocity(const glm::vec3& velocity) = 0;
		virtual void SetGravity(float gravity) = 0;
		virtual void AssignToPhysicsLayer(PhysicsLayerID layerId) = 0;

		virtual float GetRadius() const = 0;
		virtual float GetHeight() const = 0;
		virtual glm::vec3 GetAngularVelocity() const = 0;
		virtual glm::vec3 GetLinearVelocity() const = 0;
		virtual glm::vec3 GetPosition() const = 0;
		virtual glm::vec3 GetFootPosition() const = 0;

		virtual void Update(float deltaTime) = 0;
		virtual void Move(const glm::vec3& velocity) = 0;
		virtual void Jump(float jumpForce) = 0;

		virtual bool IsGrounded() const = 0;
	};
}
