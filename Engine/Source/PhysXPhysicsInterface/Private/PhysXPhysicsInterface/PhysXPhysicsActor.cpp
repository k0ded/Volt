#include "pxpch.h"

#include "PhysXPhysicsInterface/PhysXPhysicsActor.h"
#include "PhysXPhysicsInterface/PhysXPhysicsCore.h"
#include "PhysXPhysicsInterface/PhysXUtilities.h"
#include "PhysXPhysicsInterface/PhysXColliderShape.h"

namespace Volt
{
	PhysXPhysicsActor::PhysXPhysicsActor(const PhysicsActorCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		CreateActorFromCreateInfo(createInfo);
	}

	PhysXPhysicsActor::~PhysXPhysicsActor()
	{
		if (m_rigidActor)
		{
			m_rigidActor->release();
		}

		m_rigidActor = nullptr;
	}
	
	bool PhysXPhysicsActor::IsDynamic() const
	{
		return m_createInfo.bodyType == PhysicsBodyType::Dynamic;
	}
	
	bool PhysXPhysicsActor::IsKinematic() const
	{
		return IsDynamic() && m_createInfo.isKinematic;
	}
	
	void PhysXPhysicsActor::SetLinearDrag(float drag)
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to set drag of non-dynamic physics actor!");
			return;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		actor->setLinearDamping(drag);
	}
	
	void PhysXPhysicsActor::SetAngularDrag(float drag)
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to set drag of non-dynamic physics actor!");
			return;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		actor->setAngularDamping(drag);
	}
	
	void PhysXPhysicsActor::SetLinearVelocity(const glm::vec3& velocity)
	{
		if (!IsDynamic() || IsKinematic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to set velocity of non-dynamic physics actor!");
			return;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		actor->setLinearVelocity(PhysXUtilities::ToPhysXVector(velocity));
	}
	
	void PhysXPhysicsActor::SetAngularVelocity(const glm::vec3& velocity)
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to set velocity of non-dynamic physics actor!");
			return;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		actor->setAngularVelocity(PhysXUtilities::ToPhysXVector(velocity));
	}
	
	void PhysXPhysicsActor::SetMaxLinearVelocity(float velocity)
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to set velocity of non-dynamic physics actor!");
			return;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		actor->setMaxLinearVelocity(velocity);
	}
	
	void PhysXPhysicsActor::SetMaxAngularVelocity(float velocity)
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to set velocity of non-dynamic physics actor!");
			return;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		actor->setMaxAngularVelocity(velocity);
	}
	
	void PhysXPhysicsActor::SetGravityDisabled(bool disabled)
	{
		m_rigidActor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, disabled);
	}
	
	void PhysXPhysicsActor::SetMass(float mass)
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Static physics actors can't have mass!");
			return;
		}
		m_createInfo.mass = mass;

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		physx::PxRigidBodyExt::setMassAndUpdateInertia(*actor, mass);
	}
	
	void PhysXPhysicsActor::SetIsKinematic(bool isKinematic)
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Static physics actors can't be kinematic!");
			return;
		}
		m_createInfo.isKinematic = isKinematic;

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, isKinematic);
	}
	
	void PhysXPhysicsActor::SetKinematicTarget(const glm::vec3& position, const glm::quat& rotation)
	{
		if (!IsKinematic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to set kinematic target for a non-kinematic physics actor!");
			return;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		actor->setKinematicTarget(PhysXUtilities::ToPhysXTransform(position, rotation));
	}
	
	void PhysXPhysicsActor::SetPosition(const glm::vec3& position, bool autoWake)
	{
		physx::PxTransform transform = m_rigidActor->getGlobalPose();
		transform.p = PhysXUtilities::ToPhysXVector(position);

		m_rigidActor->setGlobalPose(transform);

		if (autoWake)
		{
			Wake();
		}
	}
	
	void PhysXPhysicsActor::SetRotation(const glm::quat& rotation, bool autoWake)
	{
		physx::PxTransform transform = m_rigidActor->getGlobalPose();
		transform.q = PhysXUtilities::ToPhysXQuat(rotation);

		m_rigidActor->setGlobalPose(transform);

		if (autoWake)
		{
			Wake();
		}
	}
	
	void PhysXPhysicsActor::SetLockFlag(PhysicsActorLockFlags lockFlag, bool value, bool forceAwake)
	{
		if (!IsDynamic())
		{
			return;
		}

		if (value)
		{
			m_createInfo.lockFlags |= lockFlag;
		}
		else
		{
			m_createInfo.lockFlags &= ~lockFlag;
		}

		m_rigidActor->is<physx::PxRigidDynamic>()->setRigidDynamicLockFlags(static_cast<physx::PxRigidDynamicLockFlags>(static_cast<uint8_t>(m_createInfo.lockFlags)));

		if (forceAwake)
		{
			Wake();
		}
	}
	
	void PhysXPhysicsActor::SetLockFlags(PhysicsActorLockFlags lockFlags, bool forceAwake)
	{
		m_createInfo.lockFlags = lockFlags;

		if (IsDynamic())
		{
			m_rigidActor->is<physx::PxRigidDynamic>()->setRigidDynamicLockFlags(static_cast<physx::PxRigidDynamicLockFlags>(static_cast<uint8_t>(m_createInfo.lockFlags)));
		}
	}

	void PhysXPhysicsActor::AssignToPhysicsLayer(uint32_t layerId)
	{
		//const auto data = PhysXUtilities::CreateFilterDataFromLayer(layerId, (CollisionDetectionType)m_rigidBodyData.m_collisionType);
		//myFilterData = data;

		//for (auto& collider : m_colliders)
		//{
		//	collider->SetFilterData(myFilterData);
		//}

		//myLayerId = layerId;
	}
	
	glm::vec3 PhysXPhysicsActor::GetLinearVelocity() const
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to get velocity of non-dynamic physics actor!");
			return glm::vec3(0.f);
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		return PhysXUtilities::FromPhysXVector(actor->getLinearVelocity());
	}
	
	glm::vec3 PhysXPhysicsActor::GetAngularVelocity() const
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to get velocity of non-dynamic physics actor!");
			return glm::vec3(0.f);
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		return PhysXUtilities::FromPhysXVector(actor->getAngularVelocity());
	}
	
	float PhysXPhysicsActor::GetMaxLinearVelocity() const
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to get velocity of non-dynamic physics actor!");
			return 0.f;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		return actor->getMaxLinearVelocity();
	}
	
	float PhysXPhysicsActor::GetMaxAngularVelocity() const
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to get velocity of non-dynamic physics actor!");
			return 0.f;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		return actor->getMaxAngularVelocity();
	}
	
	float PhysXPhysicsActor::GetMass() const
	{
		return !IsDynamic() ? m_createInfo.mass : m_rigidActor->is<physx::PxRigidDynamic>()->getMass();
	}
	
	glm::vec3 PhysXPhysicsActor::GetKinematicTargetPosition() const
	{
		if (!IsKinematic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to set kinematic target for a non-kinematic physics actor!");
			return glm::vec3(0.0f, 0.0f, 0.0f);
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		physx::PxTransform target;
		actor->getKinematicTarget(target);
		return PhysXUtilities::FromPhysXVector(target.p);
	}
	
	glm::quat PhysXPhysicsActor::GetKinematicTargetRotation() const
	{
		if (!IsKinematic())
		{
			VT_LOG(Warning, "Trying to set kinematic target for a non-kinematic physics actor!");
			return glm::vec3(0.0f, 0.0f, 0.0f);
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		physx::PxTransform target;
		actor->getKinematicTarget(target);
		return PhysXUtilities::FromPhysXQuat(target.q);
	}
	
	PhysicsActorID PhysXPhysicsActor::GetID() const
	{
		return m_actorId;
	}
	
	void PhysXPhysicsActor::AddForce(const glm::vec3& force, ForceMode forceMode)
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to add force to non-dynamic physics actor!");
			return;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		actor->addForce(PhysXUtilities::ToPhysXVector(force), static_cast<physx::PxForceMode::Enum>(forceMode));
	}
	
	void PhysXPhysicsActor::AddTorque(const glm::vec3& torque, ForceMode forceMode)
	{
		if (!IsDynamic())
		{
			VT_LOGC(Warning, LogPhysX, "Trying to add torque to non-dynamic physics actor!");
			return;
		}

		physx::PxRigidDynamic* actor = m_rigidActor->is<physx::PxRigidDynamic>();
		actor->addTorque(PhysXUtilities::ToPhysXVector(torque), static_cast<physx::PxForceMode::Enum>(forceMode));
	}
	
	PhysicsColliderID PhysXPhysicsActor::AddCollider(const BoxColliderCreateInfo& createInfo)
	{
		PhysicsColliderID colliderId{};
		m_colliders[colliderId] = CreateRef<PhysXBoxColliderShape>(createInfo);

		return colliderId;
	}
	
	PhysicsColliderID PhysXPhysicsActor::AddCollider(const SphereColliderCreateInfo& createInfo)
	{
		PhysicsColliderID colliderId{};
		m_colliders[colliderId] = CreateRef<PhysXSphereColliderShape>(createInfo);

		return colliderId;
	}
	
	PhysicsColliderID PhysXPhysicsActor::AddCollider(const CapsuleColliderCreateInfo& createInfo)
	{
		PhysicsColliderID colliderId{};
		m_colliders[colliderId] = CreateRef<PhysXCapsuleColliderShape>(createInfo);

		return colliderId;
	}
	
	PhysicsColliderID PhysXPhysicsActor::AddCollider(const ConvexMeshColliderCreateInfo& createInfo)
	{
		return PhysicsColliderID();
	}
	
	PhysicsColliderID PhysXPhysicsActor::AddCollider(const TriangleMeshColliderCreateInfo& createInfo)
	{
		return PhysicsColliderID();
	}
	
	void PhysXPhysicsActor::RemoveCollider(PhysicsColliderID colliderId)
	{
		if (!m_colliders.contains(colliderId))
		{
			VT_LOGC(Warning, LogPhysX, "Actor does not contain collider with ID {}!", colliderId);
			return;
		}

		m_colliders.at(colliderId)->DetachFromActor();
		m_colliders.erase(colliderId);
	}
	
	void PhysXPhysicsActor::Wake()
	{
		if (IsDynamic())
		{
			m_rigidActor->is<physx::PxRigidDynamic>()->wakeUp();
		}
	}
	
	void PhysXPhysicsActor::PutToSleep()
	{
		if (IsDynamic())
		{
			m_rigidActor->is<physx::PxRigidDynamic>()->putToSleep();
		}
	}

	void* PhysXPhysicsActor::GetHandleImpl() const
	{
		return m_rigidActor;
	}

	void PhysXPhysicsActor::CreateActorFromCreateInfo(const PhysicsActorCreateInfo& createInfo)
	{
		auto& physXCore = PhysXPhysicsCore::GetInstance()->GetCore();

		if (createInfo.bodyType == PhysicsBodyType::Static)
		{
			m_rigidActor = physXCore.createRigidStatic(PhysXUtilities::ToPhysXTransform(createInfo.initialPosition, createInfo.initialRotation));
		}
		else
		{
			m_rigidActor = physXCore.createRigidDynamic(PhysXUtilities::ToPhysXTransform(createInfo.initialPosition, createInfo.initialRotation));
		
			SetLinearDrag(createInfo.linearDrag);
			SetAngularDrag(createInfo.angularDrag);
			SetIsKinematic(createInfo.isKinematic);
			SetGravityDisabled(createInfo.disableGravity);
			SetLockFlags(createInfo.lockFlags, true);

			physx::PxRigidDynamic* physxDynamic = m_rigidActor->is<physx::PxRigidDynamic>();
			physxDynamic->setSolverIterationCounts(8, 2); // #TODO_Ivar: Where should this come from?
			physxDynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, m_createInfo.collisionDetectionType == CollisionDetectionType::Continuous);
			physxDynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, m_createInfo.collisionDetectionType == CollisionDetectionType::ContinuousSpeculative);
		
			SetMass(m_createInfo.mass);
		}

		m_rigidActor->userData = this;

		if (!createInfo.debugName.empty())
		{
			m_rigidActor->setName(createInfo.debugName.c_str());
		}

		AssignToPhysicsLayer(createInfo.physicsLayerId);
	}
}
