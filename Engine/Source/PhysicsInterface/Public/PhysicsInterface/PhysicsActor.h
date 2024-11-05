#pragma once

#include "PhysicsInterface/PhysicsTypes.h"
#include "PhysicsInterface/ColliderShape.h"
#include "PhysicsInterface/PhysicsHandleType.h"
#include "PhysicsInterface/PhysicsIDType.h"

#include <glm/glm.hpp>

namespace Volt
{
	struct PhysicsActorCreateInfo
	{
		glm::vec3 initialPosition = 0;
		glm::quat initialRotation = glm::identity<glm::quat>();

		PhysicsBodyType bodyType = PhysicsBodyType::Static;
		CollisionDetectionType collisionDetectionType = CollisionDetectionType::Discrete;
		PhysicsActorLockFlags lockFlags = PhysicsActorLockFlags::None;

		uint32_t physicsLayerId = 0;

		float mass = 1.f;
		float linearDrag = 0.01f;
		float angularDrag = 0.05f;
		
		bool isKinematic = false;
		bool disableGravity = false;

		std::string debugName;
	};

	class PhysicsActor : public PhysicsHandleType, public PhysicsIDType, public std::enable_shared_from_this<PhysicsActor>
	{
	public:
		virtual ~PhysicsActor() {}

		virtual bool IsDynamic() const = 0;
		virtual bool IsKinematic() const = 0;

		virtual void SetLinearDrag(float drag) = 0;
		virtual void SetAngularDrag(float drag) = 0;
		virtual void SetLinearVelocity(const glm::vec3& velocity) = 0;
		virtual void SetAngularVelocity(const glm::vec3& velocity) = 0;
		virtual void SetMaxLinearVelocity(float velocity) = 0;
		virtual void SetMaxAngularVelocity(float velocity) = 0;
		virtual void SetGravityDisabled(bool disabled) = 0;
		virtual void SetMass(float mass) = 0;
		virtual void SetIsKinematic(bool isKinematic) = 0;
		virtual void SetKinematicTarget(const glm::vec3& position, const glm::quat& rotation) = 0;
		virtual void SetPosition(const glm::vec3& position, bool autoWake = true) = 0;
		virtual void SetRotation(const glm::quat& rotation, bool autoWake = true) = 0;
		virtual void SetLockFlag(PhysicsActorLockFlags lockFlag, bool value, bool forceAwake = false) = 0;
		virtual void SetLockFlags(PhysicsActorLockFlags lockFlags, bool forceAwake = false) = 0;
		virtual void AssignToPhysicsLayer(uint32_t layerId) = 0;

		virtual glm::vec3 GetLinearVelocity() const = 0;
		virtual glm::vec3 GetAngularVelocity() const = 0;
		virtual float GetMaxLinearVelocity() const = 0;
		virtual float GetMaxAngularVelocity() const = 0;
		virtual float GetMass() const = 0;
		virtual glm::vec3 GetKinematicTargetPosition() const = 0;
		virtual glm::quat GetKinematicTargetRotation() const = 0;

		virtual void AddForce(const glm::vec3& force, ForceMode forceMode) = 0;
		virtual void AddTorque(const glm::vec3& torque, ForceMode forceMode) = 0;

		virtual PhysicsColliderID AddCollider(const BoxColliderCreateInfo& createInfo) = 0;
		virtual PhysicsColliderID AddCollider(const SphereColliderCreateInfo& createInfo) = 0;
		virtual PhysicsColliderID AddCollider(const CapsuleColliderCreateInfo& createInfo) = 0;
		virtual PhysicsColliderID AddCollider(const ConvexMeshColliderCreateInfo& createInfo) = 0;
		virtual PhysicsColliderID AddCollider(const TriangleMeshColliderCreateInfo& createInfo) = 0;

		virtual void RemoveCollider(PhysicsColliderID colliderId) = 0;

		virtual void Wake() = 0;
		virtual void PutToSleep() = 0;
	};
}
