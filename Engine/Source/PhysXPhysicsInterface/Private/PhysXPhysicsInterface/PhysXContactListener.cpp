#include "pxpch.h"

#include "PhysXContactListener.h"

#include <PhysicsInterface/PhysicsActor.h>
#include <PhysicsInterface/ContactListener.h>
#include <PhysicsInterface/PhysicsLayerManager.h>

namespace Volt
{
	PhysXContactListener::PhysXContactListener(Ref<ContactListener> contactListener)
		: m_contactListener(contactListener)
	{

	}

	void PhysXContactListener::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
	{
		PX_UNUSED(constraints);
		PX_UNUSED(count);
	}
	
	void PhysXContactListener::onWake(physx::PxActor** actors, physx::PxU32 count)
	{
		Vector<Ref<PhysicsActor>> result(count);

		for (uint32_t i = 0; i < count; i++)
		{
			result[i] = reinterpret_cast<PhysicsActor*>(actors[i]->userData)->shared_from_this();
		}

		m_contactListener->OnWake(result);
	}
	
	void PhysXContactListener::onSleep(physx::PxActor** actors, physx::PxU32 count)
	{
		Vector<Ref<PhysicsActor>> result(count);

		for (uint32_t i = 0; i < count; i++)
		{
			result[i] = reinterpret_cast<PhysicsActor*>(actors[i]->userData)->shared_from_this();
		}

		m_contactListener->OnSleep(result);
	}
	
	void PhysXContactListener::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
	{
		auto removedActorA = pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0;
		auto removedActorB = pairHeader.flags & physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1;

		if (removedActorA || removedActorB)
		{
			return;
		}

		Ref<PhysicsActor> actorA = reinterpret_cast<PhysicsActor*>(pairHeader.actors[0]->userData)->shared_from_this();
		Ref<PhysicsActor> actorB = reinterpret_cast<PhysicsActor*>(pairHeader.actors[1]->userData)->shared_from_this();

		ContactHeader header{};
		header.actors[0] = actorA;
		header.actors[1] = actorB;

		if (pairs->flags & physx::PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
		{
			header.contactType = PhysicsContactType::FirstTouch;
		}
		else if (pairs->flags & physx::PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
		{
			header.contactType = PhysicsContactType::LostTouch;
		}

		m_contactListener->OnContact(header);
	}
	
	void PhysXContactListener::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			if (pairs[i].flags & (physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
			{
				continue;
			}

			Ref<PhysicsActor> triggerActor = reinterpret_cast<PhysicsActor*>(pairs[i].triggerActor)->shared_from_this();
			Ref<PhysicsActor> otherActor = reinterpret_cast<PhysicsActor*>(pairs[i].otherActor)->shared_from_this();
		
			if (!triggerActor || !otherActor)
			{
				continue;
			}

			if (!g_physicsLayerManager.AreLayersColliding(triggerActor->GetAssignedPhysicsLayerID(), otherActor->GetAssignedPhysicsLayerID()))
			{
				continue;
			}

			TriggerHeader header{};
			header.triggerActor = triggerActor;
			header.otherActor = otherActor;

			if (pairs[i].status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
			{
				header.triggerType = PhysicsTriggerType::EnterTrigger;
			}
			else if (pairs[i].status == physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
			{
				header.triggerType = PhysicsTriggerType::ExitTrigger;
			}

			m_contactListener->OnTrigger(header);
		}
	}
	
	void PhysXContactListener::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
	{
	}
}
