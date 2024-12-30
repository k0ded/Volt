#pragma once

#include <CoreUtilities/Core.h>

#include <PhysX/PxPhysicsAPI.h>

namespace Volt
{
	class ContactListener;

	class PhysXContactListener : public physx::PxSimulationEventCallback
	{
	public:
		PhysXContactListener(Ref<ContactListener> contactListener);
		~PhysXContactListener() override = default;

		void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
		void onWake(physx::PxActor** actors, physx::PxU32 count) override;
		void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
		void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
		void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
		void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;

	private:
		Ref<ContactListener> m_contactListener;
	};

	class PhysXCharacterControllerContactListener : public physx::PxQueryFilterCallback
	{
	public:
		physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) override;
	};
}
