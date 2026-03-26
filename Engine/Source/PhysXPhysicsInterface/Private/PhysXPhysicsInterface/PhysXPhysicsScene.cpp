#include "pxpch.h"

#include "PhysXPhysicsInterface/PhysXPhysicsScene.h"
#include "PhysXPhysicsInterface/PhysXUtilities.h"
#include "PhysXPhysicsInterface/PhysXPhysicsCore.h"
#include "PhysXPhysicsInterface/PhysXPhysicsActor.h"
#include "PhysXPhysicsInterface/PhysXPhysicsControllerActor.h"
#include "PhysXPhysicsInterface/PhysXDebugger.h"

#include <Volt-FileSystem/Filesystem.h>

#include <PhysX/PxPhysicsAPI.h>

#include <CoreUtilities/Profiling/Profiling.h>

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
		physx::PxSceneDesc sceneDesc{ PhysXPhysicsCore::GetInstance()->GetCore().getTolerancesScale() };
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_CCD | physx::PxSceneFlag::eENABLE_PCM;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ENHANCED_DETERMINISM;
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
			m_debugger = CreateUnique<PhysXDebugger>(PhysXPhysicsCore::GetInstance()->GetFoundation());

			m_debugger->StartDebugging(Filesystem::GetWorkingDirectory(), createInfo.debugType == DebugType::LiveDebug);
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
		if (m_debugger)
		{
			m_debugger->StopDebugging();
		}

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

		m_isSimulating = true;

		for (const auto& [id, actor] : m_controllerActors)
		{
			actor->Update(timestep);
		}

		bool advanced = Advance(timestep);
		if (advanced && m_createInfo.physicsSceneAdvancedCallback)
		{
			uint32_t activeActorCount = 0;
			physx::PxActor** activeActors = m_physXScene->getActiveActors(activeActorCount);

			Vector<PhysicsActor*> updatedActors(activeActorCount);

			for (uint32_t i = 0; i < activeActorCount; i++)
			{
				PhysicsActor* actor = reinterpret_cast<PhysicsActor*>(activeActors[i]->userData);
				updatedActors[i] = actor;
			}

			m_createInfo.physicsSceneAdvancedCallback(updatedActors, m_createInfo.fixedTimestep);
		}

		m_isSimulating = false;

		for (const auto& func : m_executionQueue)
		{
			func();
		}
		m_executionQueue.clear();

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
		physx::PxFilterData data{};
		data.word0 = layerMask;

		physx::PxQueryFilterData qFilterData;
		qFilterData.flags = physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC;
		qFilterData.data = data;

		physx::PxRaycastBuffer hitInfo{};

		const glm::vec3 direction = glm::normalize(destination - origin);
		const float distance = glm::distance(destination, origin);

		bool result = m_physXScene->raycast(PhysXUtilities::ToPhysXVector(origin), PhysXUtilities::ToPhysXVector(direction), distance, hitInfo, physx::PxHitFlag::eDEFAULT, qFilterData);

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
	
	bool PhysXPhysicsScene::OverlapBox(const glm::vec3& origin, const glm::vec3& halfSize, Vector<PhysicsActorID>& outUserData, uint32_t layerMask)
	{
		physx::PxFilterData filterData{};
		filterData.word0 = layerMask;

		physx::PxQueryFilterData queryFilterData{};
		queryFilterData.flags = physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC;
		queryFilterData.data = filterData;

		std::array<physx::PxOverlapHit, MAX_OVERLAP_COLLIDERS> overlapBuffer;
		uint32_t overlapCount;

		bool hit = OverlapGeometry(origin, physx::PxBoxGeometry(halfSize.x, halfSize.y, halfSize.z), overlapBuffer, overlapCount, queryFilterData);
		if (!overlapBuffer.empty())
		{
			for (auto& overlap : overlapBuffer)
			{
				if (overlap.actor != nullptr)
				{
					auto actor = reinterpret_cast<PhysXPhysicsActor*>(overlap.actor->userData);
					if (actor)
					{
						outUserData.emplace_back(actor->GetID());
					}
				}
			}
		}

		return hit;
	}
	
	bool PhysXPhysicsScene::OverlapCapsule(const glm::vec3& origin, float radius, float halfHeight, Vector<PhysicsActorID>& outUserData, uint32_t layerMask)
	{
		physx::PxFilterData filterData{};
		filterData.word0 = layerMask;

		physx::PxQueryFilterData queryFilterData{};
		queryFilterData.flags = physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC;
		queryFilterData.data = filterData;

		std::array<physx::PxOverlapHit, MAX_OVERLAP_COLLIDERS> overlapBuffer;
		uint32_t overlapCount;

		bool hit = OverlapGeometry(origin, physx::PxCapsuleGeometry(radius, halfHeight), overlapBuffer, overlapCount, queryFilterData);
		if (!overlapBuffer.empty())
		{
			for (auto& overlap : overlapBuffer)
			{
				if (overlap.actor != nullptr)
				{
					auto actor = reinterpret_cast<PhysXPhysicsActor*>(overlap.actor->userData);
					if (actor)
					{
						outUserData.emplace_back(actor->GetID());
					}
				}
			}
		}

		return hit;
	}
	
	bool PhysXPhysicsScene::OverlapSphere(const glm::vec3& origin, float radius, Vector<PhysicsActorID>& outUserData, uint32_t layerMask)
	{
		physx::PxFilterData filterData{};

		physx::PxQueryFilterData queryFilterData{};
		queryFilterData.flags = physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC;
		queryFilterData.data = filterData;

		std::array<physx::PxOverlapHit, MAX_OVERLAP_COLLIDERS> overlapBuffer;
		uint32_t overlapCount;

		bool hit = OverlapGeometry(origin, physx::PxSphereGeometry(radius), overlapBuffer, overlapCount, queryFilterData);
		if (!overlapBuffer.empty())
		{
			for (auto& overlap : overlapBuffer)
			{
				if (overlap.actor != nullptr)
				{
					auto actor = reinterpret_cast<PhysXPhysicsActor*>(overlap.actor->userData);
					if (actor)
					{
						outUserData.emplace_back(actor->GetID());
					}
				}
			}
		}

		return hit;
	}
	
	Ref<PhysicsActor> PhysXPhysicsScene::CreateActor(const PhysicsActorCreateInfo& createInfo)
	{
		Ref<PhysicsActor> actor = CreateRef<PhysXPhysicsActor>(createInfo);
		m_actors[actor->GetID()] = actor;

		auto createFunc = [this, actor]()
		{
			m_physXScene->addActor(*actor->GetHandle<physx::PxRigidActor*>());
		};

		if (m_isSimulating)
		{
			m_executionQueue.emplace_back(createFunc);
		}
		else
		{
			createFunc();
		}

		return actor;
	}
	
	Ref<PhysicsActor> PhysXPhysicsScene::GetActor(PhysicsActorID actorId) const
	{
		VT_ENSURE(m_actors.contains(actorId));
		return m_actors.at(actorId);
	}
	
	void PhysXPhysicsScene::RemoveActor(Ref<PhysicsActor> actor)
	{
		VT_ENSURE(m_actors.contains(actor->GetID()));

		auto removeFunc = [this, actor]()
		{
			m_physXScene->removeActor(*actor->GetHandle<physx::PxRigidActor*>());
			actor->Release();
			m_actors.erase(actor->GetID());
		};

		if (m_isSimulating)
		{
			m_executionQueue.emplace_back(removeFunc);
		}
		else
		{
			removeFunc();
		}
	}
	
	void PhysXPhysicsScene::RemoveActor(PhysicsActorID actorId)
	{
		VT_ENSURE(m_actors.contains(actorId));
		RemoveActor(m_actors.at(actorId));
	}
	
	Ref<PhysicsControllerActor> PhysXPhysicsScene::CreateControllerActor(const PhysicsControllerActorCreateInfo& createInfo)
	{
		Ref<PhysicsControllerActor> controllerActor = CreateRef<PhysXPhysicsControllerActor>(createInfo, m_createInfo.gravity, m_controllerManager);
		m_controllerActors[controllerActor->GetID()] = controllerActor;

		return controllerActor;
	}
	
	Ref<PhysicsControllerActor> PhysXPhysicsScene::GetControllerActor(PhysicsActorID actorId) const
	{
		VT_ENSURE(m_controllerActors.contains(actorId));
		return m_controllerActors.at(actorId);
	}
	
	void PhysXPhysicsScene::RemoveControllerActor(Ref<PhysicsControllerActor> actor)
	{
		VT_ENSURE(m_controllerActors.contains(actor->GetID()));
		m_controllerActors.erase(actor->GetID());
		actor->Release();
	}
	
	void PhysXPhysicsScene::RemoveControllerActor(PhysicsActorID actorId)
	{
		VT_ENSURE(m_controllerActors.contains(actorId));
		auto actor = m_controllerActors.at(actorId);
		actor->Release();

		m_controllerActors.erase(actorId);
	}

	PhysicsSceneStatistics PhysXPhysicsScene::GetStatistics() const
	{
		PhysicsSceneStatistics result;
		result.actorCount = static_cast<uint32_t>(m_actors.size());
		result.controllerActorCount = static_cast<uint32_t>(m_controllerActors.size());
	
		return result;
	}

	void* PhysXPhysicsScene::GetHandleImpl() const
	{
		return m_physXScene;
	}

	bool PhysXPhysicsScene::OverlapGeometry(const glm::vec3& origin, const physx::PxGeometry& geometry, std::array<physx::PxOverlapHit, MAX_OVERLAP_COLLIDERS>& buffer, uint32_t& count, const physx::PxQueryFilterData& filterData)
	{
		physx::PxOverlapBuffer overlapBuffer(buffer.data(), MAX_OVERLAP_COLLIDERS);
		physx::PxTransform pose = PhysXUtilities::ToPhysXTransform(origin, glm::identity<glm::quat>());

		bool result = m_physXScene->overlap(geometry, pose, overlapBuffer, filterData);
		if (result)
		{
			memcpy(buffer.data(), overlapBuffer.touches, overlapBuffer.nbTouches * sizeof(physx::PxOverlapHit));
			count = overlapBuffer.nbTouches;
		}

		return result;
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
