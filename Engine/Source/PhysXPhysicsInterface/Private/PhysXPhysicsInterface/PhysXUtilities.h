#pragma once

#include <PhysicsInterface/PhysicsLayerManager.h>

#include <PhysX/PxPhysicsAPI.h>
#include <glm/glm.hpp>

namespace Volt::PhysXUtilities
{
	inline const physx::PxVec3& ToPhysXVector(const glm::vec3& vec)
	{
		return *(physx::PxVec3*)&vec;
	}

	inline const physx::PxVec4& ToPhysXVector(const glm::vec4& vec)
	{
		return *(physx::PxVec4*)&vec;
	}

	inline physx::PxExtendedVec3 ToPhysXVectorExtended(const glm::vec3& vec)
	{
		return physx::PxExtendedVec3{ (double)vec.x, (double)vec.y, (double)vec.z };
	}

	inline physx::PxQuat ToPhysXQuat(const glm::quat& quat)
	{
		return { quat.x, quat.y, quat.z, quat.w };
	}

	inline physx::PxTransform ToPhysXTransform(const glm::vec3& position, const glm::quat& rotation)
	{
		return physx::PxTransform(ToPhysXVector(position), ToPhysXQuat(rotation));
	}

	inline glm::vec3 FromPhysXVector(const physx::PxVec3& vector)
	{
		return *(glm::vec3*)&vector;
	}

	inline glm::vec3 FromPhysXVector(const physx::PxExtendedVec3& vector)
	{
		return { (float)vector.x, (float)vector.y, (float)vector.z };
	}

	inline glm::vec4 FromPhysXVector(const physx::PxVec4& vector)
	{
		return *(glm::vec4*)&vector;
	}

	inline glm::quat FromPhysXQuat(const physx::PxQuat& quat)
	{
		return { quat.w, quat.x, quat.y, quat.z };
	}

	inline glm::mat4 FromPhysXTransform(const physx::PxTransform& transform)
	{
		glm::quat rotation = FromPhysXQuat(transform.q);
		glm::vec3 position = FromPhysXVector(transform.p);
		return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
	}

	inline glm::mat4 FromPhysXMatrix(const physx::PxMat44& matrix)
	{
		return *(glm::mat4*)&matrix;
	}

	inline physx::PxBroadPhaseType::Enum ToPhysXBroadphase(BroadphaseType type)
	{
		switch (type)
		{
			case BroadphaseType::SweepAndPrune: return physx::PxBroadPhaseType::eSAP;
			case BroadphaseType::MultiBoxPrune: return physx::PxBroadPhaseType::eMBP;
			case BroadphaseType::AutomaticBoxPrune: return physx::PxBroadPhaseType::eABP;
			default:
				break;
		}

		return physx::PxBroadPhaseType::eABP;
	}

	inline physx::PxFrictionType::Enum ToPhysXFrictionType(FrictionType type)
	{
		switch (type)
		{
			case FrictionType::Patch: return physx::PxFrictionType::ePATCH;
			case FrictionType::OneDirectional: return physx::PxFrictionType::eONE_DIRECTIONAL;
			case FrictionType::TwoDirectional: return physx::PxFrictionType::eTWO_DIRECTIONAL;
			default:
				break;
		}

		return physx::PxFrictionType::ePATCH;
	}

	inline physx::PxFilterData CreateFilterData(PhysicsLayerID layerId, CollisionDetectionType collisionDetectionType)
	{
		const PhysicsLayer& physicsLayer = g_physicsLayerManager.GetLayer(layerId);

		physx::PxFilterData filterData{};
		filterData.word0 = physicsLayer.bit;
		filterData.word1 = physicsLayer.collidesWithBitMask;
		filterData.word2 = static_cast<uint32_t>(collisionDetectionType);

		return filterData;
	}
}
