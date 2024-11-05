#pragma once

#include <PhysicsInterface/PhysicsScene.h>
#include <PhysicsInterface/PhysicsSubStepper.h>

namespace physx
{
	class PxScene;
	class PxControllerManager;
}

namespace Volt
{
	class PhysXPhysicsScene : public PhysicsScene
	{
	public:
		PhysXPhysicsScene(const PhysicsSceneCreateInfo& createInfo);
		~PhysXPhysicsScene() override;

		bool Simulate(float timestep) override;

		bool RayCast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RayCastHit& outHit, uint32_t layerMask) override;
		bool LineCast(const glm::vec3& origin, const glm::vec3& destination, RayCastHit& outHit, uint32_t layerMask) override;
		bool OverlapBox(const glm::vec3& origin, const glm::vec3& halfSize, Vector<PhysicsActorID>& outUserData, uint32_t layerMask) override;
		bool OverlapCapsule(const glm::vec3& origin, float radius, float halfHeight, Vector<PhysicsActorID>& outUserData, uint32_t layerMask) override;
		bool OverlapSphere(const glm::vec3& origin, float radius, Vector<PhysicsActorID>& outUserData, uint32_t layerMask) override;

		Ref<PhysicsActor> CreateActor(const PhysicsActorCreateInfo& createInfo) override;
		Ref<PhysicsActor> GetActor(PhysicsActorID actorId) const override;
		void RemoveActor(Ref<PhysicsActor> actor) override;
		void RemoveActor(PhysicsActorID actorId) override;

		Ref<PhysicsControllerActor> CreateControllerActor(const PhysicsControllerActorCreateInfo& createInfo) override;
		Ref<PhysicsControllerActor> GetControllerActor(PhysicsActorID actorId) const override;
		void RemoveControllerActor(Ref<PhysicsControllerActor> actor) override;
		void RemoveControllerActor(PhysicsActorID actorId) override;

	private:
		bool Advance(float timeStep);

		PhysicsSceneCreateInfo m_createInfo;
		PhysicsSubStepper m_subStepper;

		vt::map<PhysicsActorID, Ref<PhysicsControllerActor>> m_controllerActors;

		physx::PxScene* m_physXScene = nullptr;
		physx::PxControllerManager* m_controllerManager = nullptr;
	};
}
