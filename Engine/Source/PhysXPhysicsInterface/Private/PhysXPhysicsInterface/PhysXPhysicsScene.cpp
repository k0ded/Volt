#include "pxpch.h"

#include "PhysXPhysicsInterface/PhysXPhysicsScene.h"
#include "PhysXPhysicsInterface/PhysXUtilities.h"
#include "PhysXPhysicsInterface/PhysXPhysicsCore.h"

#include <PhysX/PxPhysicsAPI.h>

namespace Volt
{
	inline physx::PxFilterFlags FilterShader(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, physx::PxPairFlags& pairFlags, const void*, physx::PxU32)
	{
		if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

		if (filterData0.word2 == (uint32_t)CollisionDetectionType::Continuous || filterData1.word2 == (uint32_t)CollisionDetectionType::Continuous)
		{
			pairFlags |= physx::PxPairFlag::eDETECT_DISCRETE_CONTACT;
			pairFlags |= physx::PxPairFlag::eDETECT_CCD_CONTACT;
		}

		if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
		{
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
			return physx::PxFilterFlag::eDEFAULT;
		}

		return physx::PxFilterFlag::eSUPPRESS;
	}

	PhysXPhysicsScene::PhysXPhysicsScene(const PhysicsSceneCreateInfo& createInfo)
		: m_createInfo(createInfo), m_subStepper(createInfo.fixedTimestep)
	{
		physx::PxTolerancesScale tolerances{};
		tolerances.length = 100;
		tolerances.speed = 1000;

		physx::PxSceneDesc sceneDesc{ tolerances };
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD | physx::PxSceneFlag::eENABLE_PCM;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;

		sceneDesc.gravity = PhysXUtilities::ToPhysXVector(createInfo.gravity);
		sceneDesc.broadPhaseType = PhysXUtilities::ToPhysXBroadphase(createInfo.broadphaseType);
		sceneDesc.cpuDispatcher = PhysXPhysicsCore::GetInstance()->GetCPUDispatcher();
		sceneDesc.filterShader = (physx::PxSimulationFilterShader)FilterShader;
		sceneDesc.simulationEventCallback = reinterpret_cast<physx::PxSimulationEventCallback*>(&PhysXPhysicsCore::GetInstance()->GetContactListener());
		sceneDesc.frictionType = PhysXUtilities::ToPhysXFrictionType(createInfo.frictionType);

		VT_ENSURE_MSG(sceneDesc.isValid(), "Physics scene not valid!");

		m_physXScene = PhysXPhysicsCore::GetInstance()->GetCore().createScene(sceneDesc);
		m_controllerManager = PxCreateControllerManager(*m_physXScene);
		m_controllerManager->setTessellation(true, 100.f);

		if (createInfo.debugType != DebugType::None)
		{
			m_physXScene->getScenePvdClient()->setScenePvdFlags(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS | physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES | physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS);
		}

		if (createInfo.broadphaseType != BroadphaseType::AutomaticBoxPrune)
		{
			Vector<physx::PxBounds3> regionBounds(createInfo.worldBoundsSubDivisions * createInfo.worldBoundsSubDivisions);
			physx::PxBounds3 globalBounds(PhysXUtilities::ToPhysXVector(createInfo.worldBoundsMin), PhysXUtilities::ToPhysXVector(createInfo.worldBoundsMax));
			uint32_t regionCount = physx::PxBroadPhaseExt::createRegionsFromWorldBounds(regionBounds.data(), globalBounds, createInfo.worldBoundsSubDivisions);

			for (uint32_t i = 0; i < regionCount; i++)
			{
				physx::PxBroadPhaseRegion region;
				region.mBounds = regionBounds[i];
				m_physXScene->addBroadPhaseRegion(region);
			}
		}
	}

	PhysXPhysicsScene::~PhysXPhysicsScene()
	{
		if (m_controllerManager)
		{
			m_controllerManager->release();
		}

		m_controllerManager = nullptr;

		if (m_physXScene)
		{
			m_physXScene->release();
		}

		m_physXScene = nullptr;
	}
	
	bool PhysXPhysicsScene::Simulate(float timestep)
	{
		VT_PROFILE_FUNCTION();

		for (const auto& [id, actor] : m_controllerActors)
		{
			actor->Update(timestep);
		}

		bool advanced = Advance(timestep);
		if (advanced && m_createInfo.physicsSceneAdvancedCallback)
		{
			uint32_t activeActorCount = 0;
			physx::PxActor** activeActors = m_physXScene->getActiveActors(activeActorCount);

			if (activeActorCount > 0)
			{
				Vector<Ref<PhysicsActor>> updatedActors(activeActorCount);

				for (uint32_t i = 0; i < activeActorCount; i++)
				{
					PhysicsActor* actor = reinterpret_cast<PhysicsActor*>(activeActors[i]->userData);
					updatedActors[i] = actor->shared_from_this();
				}

				m_createInfo.physicsSceneAdvancedCallback(updatedActors);
			}
		}

		return advanced;
	}
	
	bool PhysXPhysicsScene::RayCast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RayCastHit& outHit, uint32_t layerMask)
	{
		physx::PxFilterData filterData{};
		filterData.word0 = layerMask;

		physx::PxQueryFilterData queryFilterData{};
		queryFilterData.flags = physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC;
		queryFilterData.data = filterData;

		physx::PxRaycastBuffer hitInfo{};
		bool result = m_physXScene->raycast(PhysXUtilities::ToPhysXVector(origin), PhysXUtilities::ToPhysXVector(glm::normalize(direction)), maxDistance, hitInfo, physx::PxHitFlag::eDEFAULT, queryFilterData);

		if (result)
		{
			PhysicsIDType* actor = reinterpret_cast<PhysicsIDType*>(hitInfo.block.actor->userData);
			outHit.actorId = actor->GetID();
			outHit.position = PhysXUtilities::FromPhysXVector(hitInfo.block.position);
			outHit.normal = PhysXUtilities::FromPhysXVector(hitInfo.block.normal);
			outHit.distance = hitInfo.block.distance;
		}

		return result;
	}
	
	bool PhysXPhysicsScene::LineCast(const glm::vec3& origin, const glm::vec3& destination, RayCastHit& outHit, uint32_t layerMask)
	{
		return false;
	}
	
	bool PhysXPhysicsScene::OverlapBox(const glm::vec3& origin, const glm::vec3& halfSize, Vector<PhysicsActorID>& outUserData, uint32_t layerMask)
	{
		return false;
	}
	
	bool PhysXPhysicsScene::OverlapCapsule(const glm::vec3& origin, float radius, float halfHeight, Vector<PhysicsActorID>& outUserData, uint32_t layerMask)
	{
		return false;
	}
	
	bool PhysXPhysicsScene::OverlapSphere(const glm::vec3& origin, float radius, Vector<PhysicsActorID>& outUserData, uint32_t layerMask)
	{
		return false;
	}
	
	Ref<PhysicsActor> PhysXPhysicsScene::CreateActor(const PhysicsActorCreateInfo& createInfo)
	{
		return Ref<PhysicsActor>();
	}
	
	Ref<PhysicsActor> PhysXPhysicsScene::GetActor(PhysicsActorID actorId) const
	{
		return Ref<PhysicsActor>();
	}
	
	void PhysXPhysicsScene::RemoveActor(Ref<PhysicsActor> actor)
	{
	}
	
	void PhysXPhysicsScene::RemoveActor(PhysicsActorID actorId)
	{
	}
	
	Ref<PhysicsControllerActor> PhysXPhysicsScene::CreateControllerActor(const PhysicsControllerActorCreateInfo& createInfo)
	{
		return Ref<PhysicsControllerActor>();
	}
	
	Ref<PhysicsControllerActor> PhysXPhysicsScene::GetControllerActor(PhysicsActorID actorId) const
	{
		return Ref<PhysicsControllerActor>();
	}
	
	void PhysXPhysicsScene::RemoveControllerActor(Ref<PhysicsControllerActor> actor)
	{
	}
	
	void PhysXPhysicsScene::RemoveControllerActor(PhysicsActorID actorId)
	{
	}

	bool PhysXPhysicsScene::Advance(float timeStep)
	{
		uint32_t subSteps = m_subStepper.Advance(timeStep);

		for (uint32_t i = 0; i < subSteps; i++)
		{
			m_physXScene->simulate(m_createInfo.fixedTimestep);
			m_physXScene->fetchResults(true);
		}

		return subSteps > 0;
	}
}
