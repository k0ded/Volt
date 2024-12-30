#pragma once

#include <PhysicsInterface/PhysicsControllerActor.h>

namespace physx
{
	class PxController;
	class PxControllerManager;
	class PxCapsuleController;
}

namespace Volt
{
	class PhysXPhysicsControllerActor : public PhysicsControllerActor
	{
	public:
		PhysXPhysicsControllerActor(const PhysicsControllerActorCreateInfo& createInfo, const glm::vec3& gravity, physx::PxControllerManager* controllerManager);
		~PhysXPhysicsControllerActor() override;

		void Release() override;

		void SetRadius(float radius) override;
		void SetHeight(float height) override;
		void SetPosition(const glm::vec3& position) override;
		void SetFootPosition(const glm::vec3& footPosition) override;
		void SetAngularVelocity(const glm::vec3& velocity) override;
		void SetLinearVelocity(const glm::vec3& velocity) override;
		void SetGravity(float gravity) override;
		void AssignToPhysicsLayer(PhysicsLayerID layerId) override;

		float GetRadius() const override;
		float GetHeight() const override;
		glm::vec3 GetAngularVelocity() const override;
		glm::vec3 GetLinearVelocity() const override;
		glm::vec3 GetPosition() const override;
		glm::vec3 GetFootPosition() const override;
		PhysicsActorID GetID() const override;

		void Update(float deltaTime) override;
		void Move(const glm::vec3& velocity) override;
		void Jump(float jumpForce) override;

		bool IsGrounded() const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateActorFromCreateInfo(const PhysicsControllerActorCreateInfo& createInfo, physx::PxControllerManager* controllerManager);

		float m_gravity;

		PhysicsControllerActorCreateInfo m_createInfo;
		float m_gravityVelocity = 0.f;
		glm::vec3 m_frameMovement = 0.f;
		uint32_t m_currentCollisionFlags = 0;
		PhysicsActorID m_actorId;
		PhysicsLayerID m_layerId = 0;

		physx::PxCapsuleController* m_controller = nullptr;
	};
}
