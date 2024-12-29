#include "vppch.h"

#include "Volt-Physics/EntityPhysicsScene.h"
#include "Volt-Physics/PhysicsSubSystem.h"
#include "Volt-Physics/Components.h"
#include "Volt-Physics/RigidbodyComponent.h"

#include <EntitySystem/EntityScene.h>

#include <SubSystem/SubSystemManager.h>

namespace Volt
{
	EntityPhysicsScene::EntityPhysicsScene(EntityScene& entityScene)
		: m_entityScene(entityScene)
	{
		auto physicsCore = SubSystemManager::GetSubSystem<PhysicsSubSystem>()->GetPhysicsCore();

		PhysicsSceneCreateInfo sceneCreateInfo{};
		sceneCreateInfo.physicsSceneAdvancedCallback = [&](const Vector<Ref<PhysicsActor>> actors, float timestep)
		{
			for (const auto& actor : actors)
			{
				VT_ENSURE(m_physicsActorToEntity.contains(actor->GetID()));

				EntityID entityId = m_physicsActorToEntity.at(actor->GetID());
				auto entity = m_entityScene.GetEntityHelperFromEntityID(entityId);
				auto transform = actor->GetTransform();

				entity.SetPosition(transform.translation);
				entity.SetRotation(transform.rotation);
			}
		};

		m_physicsScene = physicsCore->CreateScene(sceneCreateInfo);

		auto& registry = m_entityScene.GetRegistry();

		// Rigid bodies
		{
			auto view = registry.view<const TagComponent, RigidbodyComponent>();
			view.each([&](const entt::entity id, const TagComponent& tag, RigidbodyComponent& rigidbody)
			{
				auto entity = m_entityScene.GetEntityHelperFromEntityHandle(id);

				PhysicsActorCreateInfo createInfo{};
				createInfo.initialPosition = entity.GetPosition();
				createInfo.initialRotation = entity.GetRotation();
				createInfo.bodyType = rigidbody.GetBodyType();
				createInfo.collisionDetectionType = rigidbody.GetCollisionDetectionType();
				createInfo.lockFlags = static_cast<PhysicsActorLockFlags>(rigidbody.GetLockFlags());
				createInfo.layerId = rigidbody.GetLayerId();
				createInfo.mass = rigidbody.GetMass();
				createInfo.linearDrag = rigidbody.GetLinearDrag();
				createInfo.angularDrag = rigidbody.GetAngularDrag();
				createInfo.debugName = tag.tag;

				auto physicsActor = m_physicsScene->CreateActor(createInfo);

				rigidbody.actorId = physicsActor->GetID();
				m_physicsActorToEntity[physicsActor->GetID()] = entity.GetID();

				if (entity.HasComponent<BoxColliderComponent>())
				{
					auto& boxComp = entity.GetComponent<BoxColliderComponent>();

					BoxColliderCreateInfo colliderCreateInfo{};
					colliderCreateInfo.halfSize = boxComp.halfSize;
					colliderCreateInfo.isTrigger = boxComp.isTrigger;
					colliderCreateInfo.offset = boxComp.offset;
					colliderCreateInfo.scale = entity.GetScale();
					colliderCreateInfo.targetActor = physicsActor.get();
					colliderCreateInfo.physicalMaterial = physicsCore->CreateMaterial({});

					boxComp.colliderId = physicsActor->AddCollider(colliderCreateInfo);
				}

				if (entity.HasComponent<SphereColliderComponent>())
				{
					auto& sphereComp = entity.GetComponent<SphereColliderComponent>();

					SphereColliderCreateInfo colliderCreateInfo{};
					colliderCreateInfo.radius = sphereComp.radius;
					colliderCreateInfo.isTrigger = sphereComp.isTrigger;
					colliderCreateInfo.offset = sphereComp.offset;
					colliderCreateInfo.scale = entity.GetScale();
					colliderCreateInfo.targetActor = physicsActor.get();
					colliderCreateInfo.physicalMaterial = physicsCore->CreateMaterial({});

					sphereComp.colliderId = physicsActor->AddCollider(colliderCreateInfo);
				}

				if (entity.HasComponent<CapsuleColliderComponent>())
				{
					auto& capsuleComp = entity.GetComponent<CapsuleColliderComponent>();

					CapsuleColliderCreateInfo colliderCreateInfo{};
					colliderCreateInfo.height = capsuleComp.height;
					colliderCreateInfo.radius = capsuleComp.radius;
					colliderCreateInfo.isTrigger = capsuleComp.isTrigger;
					colliderCreateInfo.offset = capsuleComp.offset;
					colliderCreateInfo.scale = entity.GetScale();
					colliderCreateInfo.targetActor = physicsActor.get();
					colliderCreateInfo.physicalMaterial = physicsCore->CreateMaterial({});

					capsuleComp.colliderId = physicsActor->AddCollider(colliderCreateInfo);
				}
			});
		}

		// Character controller
		{
			auto view = registry.view<const TagComponent, CharacterControllerComponent>();
			view.each([&](const entt::entity id, const TagComponent& tag, CharacterControllerComponent& comp)
			{
				auto entity = m_entityScene.GetEntityHelperFromEntityHandle(id);

				PhysicsControllerActorCreateInfo createInfo{};
				createInfo.initialPosition = entity.GetPosition();
				createInfo.slopeLimitDegrees = comp.slopeLimit;
				createInfo.invisibleWallHeight = comp.invisibleWallHeight;
				createInfo.maxJumpHeight = comp.maxJumpHeight;
				createInfo.contactOffset = comp.contactOffset;
				createInfo.stepOffset = comp.stepOffset;
				createInfo.density = comp.density;
				createInfo.layerId = comp.layer;
				createInfo.disableGravity = !comp.hasGravity;
				createInfo.nonWalkableMode = comp.climbingMode;
				createInfo.debugName = tag.tag;

				comp.actorId = m_physicsScene->CreateControllerActor(createInfo)->GetID();
			});
		}
	}

	void EntityPhysicsScene::Update(float deltaTime)
	{
		m_physicsScene->Simulate(deltaTime);
	}
}
