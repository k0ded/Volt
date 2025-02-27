#include "pxpch.h"
#include "PhysXPhysicsInterface/PhysXColliderShape.h"
#include "PhysXPhysicsInterface/PhysXPhysicsActor.h"
#include "PhysXPhysicsInterface/PhysXPhysicsMaterial.h"
#include "PhysXPhysicsInterface/PhysXUtilities.h"

#include <PhysicsInterface/PhysicsLayerManager.h>

#include <PhysX/PxPhysicsAPI.h>

namespace Volt
{
	PhysXBoxColliderShape::PhysXBoxColliderShape(const BoxColliderCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		const glm::vec3 halfSize = createInfo.halfSize * createInfo.scale;

		physx::PxBoxGeometry geometry = physx::PxBoxGeometry(halfSize.x, halfSize.y, halfSize.z);
		physx::PxRigidActor* pxRigidActor = createInfo.targetActor->GetHandle<physx::PxRigidActor*>();
		physx::PxMaterial* pxMaterial = createInfo.physicalMaterial->GetHandle<physx::PxMaterial*>();

		m_shape = physx::PxRigidActorExt::createExclusiveShape(*pxRigidActor, geometry, *pxMaterial);
		m_shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, !createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, createInfo.isTrigger);
		m_shape->setLocalPose(PhysXUtilities::ToPhysXTransform(createInfo.offset, glm::identity<glm::quat>()));
		m_shape->userData = this;

		AssignToPhysicsLayer(createInfo.targetActor->GetAssignedPhysicsLayerID());
	}
	
	PhysXBoxColliderShape::~PhysXBoxColliderShape()
	{
		// Note: Release not required as detach decrements the ref counter
		m_shape = nullptr;
	}
	
	void PhysXBoxColliderShape::SetIsTrigger(bool isTrigger)
	{
		m_createInfo.isTrigger = isTrigger;
	
		m_shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, !m_createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_createInfo.isTrigger);
	}
	
	void PhysXBoxColliderShape::SetOffset(const glm::vec3& offset)
	{
		m_createInfo.offset = offset;
		m_shape->setLocalPose(PhysXUtilities::ToPhysXTransform(m_createInfo.offset, glm::identity<glm::quat>()));
	}

	void PhysXBoxColliderShape::SetScale(const glm::vec3& scale)
	{
		m_createInfo.scale = scale;

		const glm::vec3 halfSize = m_createInfo.halfSize * m_createInfo.scale;
		physx::PxBoxGeometry geometry = physx::PxBoxGeometry(halfSize.x, halfSize.y, halfSize.z);
		m_shape->setGeometry(geometry);
	}
	
	void PhysXBoxColliderShape::SetHalfSize(const glm::vec3& halfSize)
	{
		m_createInfo.halfSize = halfSize;

		const glm::vec3 scaledHalfSize = m_createInfo.halfSize * m_createInfo.scale;
		physx::PxBoxGeometry geometry = physx::PxBoxGeometry(scaledHalfSize.x, scaledHalfSize.y, scaledHalfSize.z);
		m_shape->setGeometry(geometry);
	}

	void PhysXBoxColliderShape::AssignToPhysicsLayer(PhysicsLayerID layerId)
	{
		const auto filterData = PhysXUtilities::CreateFilterData(layerId, m_createInfo.targetActor->GetCollisionDetectionType());

		m_shape->setSimulationFilterData(filterData);
		m_shape->setQueryFilterData(filterData);
	}

	void PhysXBoxColliderShape::DetachFromActor()
	{
		m_createInfo.targetActor->GetHandle<physx::PxRigidActor*>()->detachShape(*m_shape);
	}

	PhysXSphereColliderShape::PhysXSphereColliderShape(const SphereColliderCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		const float maxScale = glm::max(createInfo.scale.x, glm::max(createInfo.scale.y, createInfo.scale.z));

		physx::PxSphereGeometry geometry = physx::PxSphereGeometry(maxScale * createInfo.radius);
		physx::PxRigidActor* pxRigidActor = createInfo.targetActor->GetHandle<physx::PxRigidActor*>();
		physx::PxMaterial* pxMaterial = createInfo.physicalMaterial->GetHandle<physx::PxMaterial*>();

		m_shape = physx::PxRigidActorExt::createExclusiveShape(*pxRigidActor, geometry, *pxMaterial);
		m_shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, !createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, createInfo.isTrigger);
		m_shape->setLocalPose(PhysXUtilities::ToPhysXTransform(createInfo.offset, glm::identity<glm::quat>()));
		m_shape->userData = this;

		AssignToPhysicsLayer(createInfo.targetActor->GetAssignedPhysicsLayerID());
	}
	
	PhysXSphereColliderShape::~PhysXSphereColliderShape()
	{
		// Note: Release not required as detach decrements the ref counter
		m_shape = nullptr;
	}
	
	void PhysXSphereColliderShape::SetIsTrigger(bool isTrigger)
	{
		m_createInfo.isTrigger = isTrigger;

		m_shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, !m_createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_createInfo.isTrigger);
	}
	
	void PhysXSphereColliderShape::SetOffset(const glm::vec3& offset)
	{
		m_createInfo.offset = offset;
		m_shape->setLocalPose(PhysXUtilities::ToPhysXTransform(m_createInfo.offset, glm::identity<glm::quat>()));
	}
	
	void PhysXSphereColliderShape::SetScale(const glm::vec3& scale)
	{
		m_createInfo.scale = scale;

		const float maxScale = glm::max(m_createInfo.scale.x, glm::max(m_createInfo.scale.y, m_createInfo.scale.z));
		physx::PxSphereGeometry geometry = physx::PxSphereGeometry(maxScale * m_createInfo.radius);
		m_shape->setGeometry(geometry);
	}
	
	void PhysXSphereColliderShape::SetRadius(float radius)
	{
		m_createInfo.radius = radius;

		const float maxScale = glm::max(m_createInfo.scale.x, glm::max(m_createInfo.scale.y, m_createInfo.scale.z));
		physx::PxSphereGeometry geometry = physx::PxSphereGeometry(maxScale * m_createInfo.radius);
		m_shape->setGeometry(geometry);
	}

	void PhysXSphereColliderShape::AssignToPhysicsLayer(PhysicsLayerID layerId)
	{
		const auto filterData = PhysXUtilities::CreateFilterData(layerId, m_createInfo.targetActor->GetCollisionDetectionType());

		m_shape->setSimulationFilterData(filterData);
		m_shape->setQueryFilterData(filterData);
	}

	void PhysXSphereColliderShape::DetachFromActor()
	{
		m_createInfo.targetActor->GetHandle<physx::PxRigidActor*>()->detachShape(*m_shape);
	}

	PhysXCapsuleColliderShape::PhysXCapsuleColliderShape(const CapsuleColliderCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		const float radiusScale = glm::max(createInfo.scale.x, createInfo.scale.z);
		const float heightScale = createInfo.scale.y;

		physx::PxCapsuleGeometry geometry = physx::PxCapsuleGeometry(createInfo.radius * radiusScale, (createInfo.height / 2.f) * heightScale);
		physx::PxRigidActor* pxRigidActor = createInfo.targetActor->GetHandle<physx::PxRigidActor*>();
		physx::PxMaterial* pxMaterial = createInfo.physicalMaterial->GetHandle<physx::PxMaterial*>();

		m_shape = physx::PxRigidActorExt::createExclusiveShape(*pxRigidActor, geometry, *pxMaterial);
		m_shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, !createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, createInfo.isTrigger);
		m_shape->setLocalPose(PhysXUtilities::ToPhysXTransform(createInfo.offset, glm::rotate(glm::identity<glm::quat>(), glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f))));
		m_shape->userData = this;

		AssignToPhysicsLayer(createInfo.targetActor->GetAssignedPhysicsLayerID());
	}
	
	PhysXCapsuleColliderShape::~PhysXCapsuleColliderShape()
	{
		// Note: Release not required as detach decrements the ref counter
		m_shape = nullptr;
	}
	
	void PhysXCapsuleColliderShape::SetIsTrigger(bool isTrigger)
	{
		m_createInfo.isTrigger = isTrigger;

		m_shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, !m_createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, !m_createInfo.isTrigger);
		m_shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, m_createInfo.isTrigger);
	}
	
	void PhysXCapsuleColliderShape::SetOffset(const glm::vec3& offset)
	{
		m_createInfo.offset = offset;
		m_shape->setLocalPose(PhysXUtilities::ToPhysXTransform(m_createInfo.offset, glm::rotate(glm::identity<glm::quat>(), glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f))));
	}
	
	void PhysXCapsuleColliderShape::SetScale(const glm::vec3& scale)
	{
		m_createInfo.scale = scale;

		const float radiusScale = glm::max(m_createInfo.scale.x, m_createInfo.scale.z);
		const float heightScale = m_createInfo.scale.y;

		physx::PxCapsuleGeometry geometry = physx::PxCapsuleGeometry(m_createInfo.radius * radiusScale, (m_createInfo.height / 2.f) * heightScale);
		m_shape->setGeometry(geometry);
	}
	
	void PhysXCapsuleColliderShape::SetRadius(float radius)
	{
		m_createInfo.radius = radius;

		const float radiusScale = glm::max(m_createInfo.scale.x, m_createInfo.scale.z);
		const float heightScale = m_createInfo.scale.y;

		physx::PxCapsuleGeometry geometry = physx::PxCapsuleGeometry(m_createInfo.radius * radiusScale, (m_createInfo.height / 2.f) * heightScale);
		m_shape->setGeometry(geometry);
	}
	
	void PhysXCapsuleColliderShape::SetHeight(float height)
	{
		m_createInfo.height = height;

		const float radiusScale = glm::max(m_createInfo.scale.x, m_createInfo.scale.z);
		const float heightScale = m_createInfo.scale.y;

		physx::PxCapsuleGeometry geometry = physx::PxCapsuleGeometry(m_createInfo.radius * radiusScale, (m_createInfo.height / 2.f) * heightScale);
		m_shape->setGeometry(geometry);
	}

	void PhysXCapsuleColliderShape::AssignToPhysicsLayer(PhysicsLayerID layerId)
	{
		const auto filterData = PhysXUtilities::CreateFilterData(layerId, m_createInfo.targetActor->GetCollisionDetectionType());

		m_shape->setSimulationFilterData(filterData);
		m_shape->setQueryFilterData(filterData);
	}

	void PhysXCapsuleColliderShape::DetachFromActor()
	{
		m_createInfo.targetActor->GetHandle<physx::PxRigidActor*>()->detachShape(*m_shape);
	}
}
