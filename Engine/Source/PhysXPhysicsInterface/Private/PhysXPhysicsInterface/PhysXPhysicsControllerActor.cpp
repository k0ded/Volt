#include "pxpch.h"

#include "PhysXPhysicsInterface/PhysXPhysicsControllerActor.h"
#include "PhysXPhysicsInterface/PhysXUtilities.h"
#include "PhysXPhysicsInterface/PhysXContactListener.h"

#include <PhysX/PxPhysicsAPI.h>

namespace Volt
{
	static PhysXCharacterControllerContactListener s_contactListener;

	PhysXPhysicsControllerActor::PhysXPhysicsControllerActor(const PhysicsControllerActorCreateInfo& createInfo, const glm::vec3& gravity, physx::PxControllerManager* controllerManager)
		: m_createInfo(createInfo), m_gravity(gravity.y)
	{
		CreateActorFromCreateInfo(createInfo, controllerManager);
	}

	PhysXPhysicsControllerActor::~PhysXPhysicsControllerActor()
	{
		Release();
	}

	void PhysXPhysicsControllerActor::Release()
	{
		if (m_controller)
		{
			m_controller->release();
		}

		m_controller = nullptr;
	}

	void PhysXPhysicsControllerActor::SetRadius(float radius)
	{
		m_controller->setRadius(radius);
		m_createInfo.radius = radius;
	}

	void PhysXPhysicsControllerActor::SetHeight(float height)
	{
		m_controller->setHeight(height);
		m_controller->resize(height);
		m_createInfo.height = height;
	}

	void PhysXPhysicsControllerActor::SetPosition(const glm::vec3& position)
	{
		m_controller->setPosition(PhysXUtilities::ToPhysXVectorExtended(position));
	}
	
	void PhysXPhysicsControllerActor::SetFootPosition(const glm::vec3& footPosition)
	{
		m_controller->setFootPosition(PhysXUtilities::ToPhysXVectorExtended(footPosition));
	}

	void PhysXPhysicsControllerActor::SetAngularVelocity(const glm::vec3& velocity)
	{
		m_controller->getActor()->is<physx::PxRigidDynamic>()->setAngularVelocity(PhysXUtilities::ToPhysXVector(velocity));
	}
	
	void PhysXPhysicsControllerActor::SetLinearVelocity(const glm::vec3& velocity)
	{
		m_gravityVelocity = -velocity.y;
		m_controller->getActor()->is<physx::PxRigidDynamic>()->setLinearVelocity(PhysXUtilities::ToPhysXVector(velocity));
	}
	
	void PhysXPhysicsControllerActor::SetGravity(float gravity)
	{
		m_gravity = gravity;
	}

	void PhysXPhysicsControllerActor::AssignToPhysicsLayer(PhysicsLayerID layerId)
	{
		if (m_layerId == layerId)
		{
			return;
		}

		const auto filterData = PhysXUtilities::CreateFilterData(layerId, CollisionDetectionType::Continuous);

		const uint32_t shapeCount = m_controller->getActor()->getNbShapes();
		Vector<physx::PxShape*> shapes(shapeCount);

		m_controller->getActor()->getShapes(shapes.data(), shapeCount);
		for (const auto& shape : shapes)
		{
			shape->setSimulationFilterData(filterData);
			shape->setQueryFilterData(filterData);
		}

		m_layerId = layerId;
	}
	
	float PhysXPhysicsControllerActor::GetRadius() const
	{
		return m_createInfo.radius;
	}
	
	float PhysXPhysicsControllerActor::GetHeight() const
	{
		return m_createInfo.height;
	}
	
	glm::vec3 PhysXPhysicsControllerActor::GetAngularVelocity() const
	{
		return PhysXUtilities::FromPhysXVector(m_controller->getActor()->is<physx::PxRigidDynamic>()->getAngularVelocity());
	}
	
	glm::vec3 PhysXPhysicsControllerActor::GetLinearVelocity() const
	{
		return PhysXUtilities::FromPhysXVector(m_controller->getActor()->is<physx::PxRigidDynamic>()->getLinearVelocity());
	}
	
	glm::vec3 PhysXPhysicsControllerActor::GetPosition() const
	{
		return PhysXUtilities::FromPhysXVector(m_controller->getPosition());
	}
	
	glm::vec3 PhysXPhysicsControllerActor::GetFootPosition() const
	{
		return PhysXUtilities::FromPhysXVector(m_controller->getFootPosition());
	}
	
	PhysicsActorID PhysXPhysicsControllerActor::GetID() const
	{
		return m_actorId;
	}
	
	void PhysXPhysicsControllerActor::Update(float deltaTime)
	{
		if (!m_createInfo.disableGravity)
		{
			m_gravityVelocity += m_gravity * deltaTime;
		}

		auto filterData = PhysXUtilities::CreateFilterData(m_layerId, CollisionDetectionType::Continuous);

		physx::PxControllerFilters filters{};
		filters.mCCTFilterCallback = nullptr;
		filters.mFilterCallback = &s_contactListener;
		filters.mFilterData = &filterData;
	
		glm::vec3 finalVelocity = m_frameMovement - glm::vec3(0.f, 1.f, 0.f) * m_gravityVelocity * deltaTime;

		m_currentCollisionFlags = m_controller->move(PhysXUtilities::ToPhysXVector(finalVelocity), 0.f, deltaTime, filters);

		if (IsGrounded())
		{
			m_gravityVelocity = m_gravity * 0.01f;
		}

		m_frameMovement = 0.f;
	}

	void PhysXPhysicsControllerActor::Move(const glm::vec3& velocity)
	{
		m_frameMovement += velocity;
	}
	
	void PhysXPhysicsControllerActor::Jump(float jumpForce)
	{
		m_gravityVelocity = -1.f * jumpForce;
	}
	
	bool PhysXPhysicsControllerActor::IsGrounded() const
	{
		return m_currentCollisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN;
	}
	
	void* PhysXPhysicsControllerActor::GetHandleImpl() const
	{
		return m_controller;
	}

	void PhysXPhysicsControllerActor::CreateActorFromCreateInfo(const PhysicsControllerActorCreateInfo& createInfo, physx::PxControllerManager* controllerManager)
	{
		physx::PxCapsuleControllerDesc controllerDesc{};
		controllerDesc.upDirection = { 0.f, 1.f, 0.f };
		controllerDesc.slopeLimit = std::max(0.f, glm::cos(glm::radians(createInfo.slopeLimitDegrees)));
		controllerDesc.invisibleWallHeight = createInfo.invisibleWallHeight;
		controllerDesc.maxJumpHeight = createInfo.maxJumpHeight;
		controllerDesc.contactOffset = createInfo.contactOffset;
		controllerDesc.stepOffset = createInfo.stepOffset;
		controllerDesc.density = createInfo.density;
		controllerDesc.userData = this;
		controllerDesc.radius = createInfo.radius;
		controllerDesc.height = createInfo.height;
		controllerDesc.nonWalkableMode = createInfo.nonWalkableMode == PhysicsControllerActorNonWalkableMode::PreventClimbing ? physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING : physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
		controllerDesc.position = PhysXUtilities::ToPhysXVectorExtended(createInfo.initialPosition);
		controllerDesc.material = createInfo.physicalMaterial->GetHandle<physx::PxMaterial*>();

		m_controller = reinterpret_cast<physx::PxCapsuleController*>(controllerManager->createController(controllerDesc));
		
		if (!createInfo.debugName.empty())
		{
			m_controller->getActor()->setName(createInfo.debugName.c_str());
		}
	}
}
