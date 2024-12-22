#pragma once

#include <PhysicsInterface/PhysicsActor.h>

#include <CoreUtilities/Containers/Map.h>

namespace physx
{
	class PxRigidActor;
}

namespace Volt
{
	class PhysXPhysicsActor : public PhysicsActor
	{
	public:
		PhysXPhysicsActor(const PhysicsActorCreateInfo& createInfo);
		~PhysXPhysicsActor() override;

		void Release() override;

		bool IsDynamic() const override;
		bool IsKinematic() const override;

		void SetLinearDrag(float drag) override;
		void SetAngularDrag(float drag) override;
		void SetLinearVelocity(const glm::vec3& velocity) override;
		void SetAngularVelocity(const glm::vec3& velocity) override;
		void SetMaxLinearVelocity(float velocity) override;
		void SetMaxAngularVelocity(float velocity) override;
		void SetGravityDisabled(bool disabled) override;
		void SetMass(float mass) override;
		void SetIsKinematic(bool isKinematic) override;
		void SetKinematicTarget(const glm::vec3& position, const glm::quat& rotation) override;
		void SetPosition(const glm::vec3& position, bool autoWake /* = true */) override;
		void SetRotation(const glm::quat& rotation, bool autoWake /* = true */) override;
		void SetLockFlag(PhysicsActorLockFlags lockFlag, bool value, bool forceAwake /* = false */) override;
		void SetLockFlags(PhysicsActorLockFlags lockFlags, bool forceAwake /* = false */) override;
		void AssignToPhysicsLayer(PhysicsLayerID layerId) override;
		PhysicsLayerID GetAssignedPhysicsLayerID() const override;

		glm::vec3 GetLinearVelocity() const override;
		glm::vec3 GetAngularVelocity() const override;
		float GetMaxLinearVelocity() const override;
		float GetMaxAngularVelocity() const override;
		float GetMass() const override;
		glm::vec3 GetKinematicTargetPosition() const override;
		glm::quat GetKinematicTargetRotation() const override;
		PhysicsActorID GetID() const override;
		CollisionDetectionType GetCollisionDetectionType() const override;

		void AddForce(const glm::vec3& force, ForceMode forceMode) override;
		void AddTorque(const glm::vec3& torque, ForceMode forceMode) override;

		PhysicsColliderID AddCollider(const BoxColliderCreateInfo& createInfo) override;
		PhysicsColliderID AddCollider(const SphereColliderCreateInfo& createInfo) override;
		PhysicsColliderID AddCollider(const CapsuleColliderCreateInfo& createInfo) override;
		PhysicsColliderID AddCollider(const ConvexMeshColliderCreateInfo& createInfo) override;
		PhysicsColliderID AddCollider(const TriangleMeshColliderCreateInfo& createInfo) override;

		void RemoveCollider(PhysicsColliderID colliderId) override;

		void Wake() override;
		void PutToSleep() override;

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateActorFromCreateInfo(const PhysicsActorCreateInfo& createInfo);

		PhysicsActorCreateInfo m_createInfo;
		physx::PxRigidActor* m_rigidActor = nullptr;
		PhysicsActorID m_actorId;
		PhysicsLayerID m_layerId = 0;

		vt::map<PhysicsColliderID, Ref<ColliderShape>> m_colliders;
	};
}
