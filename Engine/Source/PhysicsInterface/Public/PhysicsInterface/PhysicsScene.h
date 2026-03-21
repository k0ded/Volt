#pragma once

#include "PhysicsInterface/PhysicsTypes.h"
#include "PhysicsInterface/RayCast.h"
#include "PhysicsInterface/PhysicsActor.h"
#include "PhysicsInterface/PhysicsControllerActor.h"
#include "PhysicsInterface/PhysicsHandleType.h"
#include "PhysicsInterface/PhysicsLayer.h"

#include <CoreUtilities/Containers/Vector.h>

#include <functional>

namespace Volt
{
	using PhysicsSceneAdvancedCallback = std::function<void(const Vector<PhysicsActor*>, float timestep)>;

	struct PhysicsSceneCreateInfo
	{
		float fixedTimestep = 1.f / 60.f;
		glm::vec3 gravity = { 0.f, -981.f, 0.f };

		BroadphaseType broadphaseType = BroadphaseType::AutomaticBoxPrune;
		FrictionType frictionType = FrictionType::Patch;

		glm::vec3 worldBoundsMin = glm::vec3{ -100000.f };
		glm::vec3 worldBoundsMax = glm::vec3{ 100000.f };

		uint32_t worldBoundsSubDivisions = 2;
		uint32_t solverIterations = 8;
		uint32_t solverVelocityIterations = 2;

		bool debugOnPlay = true;
		DebugType debugType = DebugType::LiveDebug;

		PhysicsSceneAdvancedCallback physicsSceneAdvancedCallback;
	};

	struct PhysicsSceneStatistics
	{
		uint32_t actorCount;
		uint32_t controllerActorCount;
	};

	class PhysicsScene : public PhysicsHandleType
	{
	public:
		virtual ~PhysicsScene() {}

		virtual bool Simulate(float timestep) = 0;

		virtual bool RayCast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, RayCastHit& outHit, uint32_t layerMask = 0) = 0;
		virtual bool LineCast(const glm::vec3& origin, const glm::vec3& destination, RayCastHit& outHit, uint32_t layerMask = 0) = 0;
		virtual bool OverlapBox(const glm::vec3& origin, const glm::vec3& halfSize, Vector<PhysicsActorID>& outUserData, uint32_t layerMask = 0) = 0;
		virtual bool OverlapCapsule(const glm::vec3& origin, float radius, float halfHeight, Vector<PhysicsActorID>& outUserData, uint32_t layerMask = 0) = 0;
		virtual bool OverlapSphere(const glm::vec3& origin, float radius, Vector<PhysicsActorID>& outUserData, uint32_t layerMask = 0) = 0;

		virtual Ref<PhysicsActor> CreateActor(const PhysicsActorCreateInfo& createInfo) = 0;
		virtual Ref<PhysicsActor> GetActor(PhysicsActorID actorId) const = 0;
		virtual void RemoveActor(Ref<PhysicsActor> actor) = 0;
		virtual void RemoveActor(PhysicsActorID actorId) = 0;

		virtual Ref<PhysicsControllerActor> CreateControllerActor(const PhysicsControllerActorCreateInfo& createInfo) = 0;
		virtual Ref<PhysicsControllerActor> GetControllerActor(PhysicsActorID actorId) const = 0;
		virtual void RemoveControllerActor(Ref<PhysicsControllerActor> actor) = 0;
		virtual void RemoveControllerActor(PhysicsActorID actorId) = 0;

		virtual PhysicsSceneStatistics GetStatistics() const = 0;
	};
}
