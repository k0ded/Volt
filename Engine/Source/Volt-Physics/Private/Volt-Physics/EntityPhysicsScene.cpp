#include "vppch.h"

#include "Volt-Physics/EntityPhysicsScene.h"
#include "Volt-Physics/PhysicsSubSystem.h"
#include "Volt-Physics/ColliderComponents.h"
#include "Volt-Physics/CharacterControllerComponent.h"
#include "Volt-Physics/RigidbodyComponent.h"

#include <EntitySystem/EntityScene.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	EntityPhysicsScene::EntityPhysicsScene(EntityScene& entityScene)
		: m_entityScene(entityScene)
	{
		auto physicsCore = SubSystemManager::GetSubSystem<PhysicsSubSystem>()->GetPhysicsCore();

		m_transformChangedCallbackID = entityScene.RegisterTransformChangedCallback([&](Entity entity) 
		{
			if (m_isHandlingPhysicsUpdate)
			{
				return;
			}

			if (m_entityToPhysicsActor.contains(entity.GetID()))
			{
				auto actor = m_physicsScene->GetActor(m_entityToPhysicsActor.at(entity.GetID()));
				actor->SetPosition(entity.GetPosition(), false);
				actor->SetRotation(entity.GetRotation());
			}

			if (m_entityToPhysicsControllerActor.contains(entity.GetID()))
			{
				auto controllerActor = m_physicsScene->GetControllerActor(m_entityToPhysicsControllerActor.at(entity.GetID()));
				controllerActor->SetFootPosition(entity.GetPosition());
			}
		});

		m_entityDestroyedCallbackID = entityScene.RegisterEntityDestroyedCallback([&](Entity entity) 
		{
			if (m_entityToPhysicsActor.contains(entity.GetID()))
			{
				m_physicsActorToEntity.erase(m_entityToPhysicsActor.at(entity.GetID()));
				m_entityToPhysicsActor.erase(entity.GetID());
			}

			if (m_entityToPhysicsControllerActor.contains(entity.GetID()))
			{
				m_entityToPhysicsControllerActor.erase(entity.GetID());
			}
		});

		PhysicsSceneCreateInfo sceneCreateInfo{};
		sceneCreateInfo.physicsSceneAdvancedCallback = [&](const Vector<PhysicsActor*> actors, float timestep)
		{
			m_isHandlingPhysicsUpdate = true;

			for (const auto& actor : actors)
			{
				VT_ENSURE(m_physicsActorToEntity.contains(actor->GetID()));

				EntityID entityId = m_physicsActorToEntity.at(actor->GetID());
				auto entity = m_entityScene.GetEntityFromID(entityId);
				auto transform = actor->GetTransform();

				entity.SetPosition(transform.translation);
				entity.SetRotation(transform.rotation);
			}

			m_entityScene.FixedUpdate(timestep);
			m_isHandlingPhysicsUpdate = false;
		};

		m_physicsScene = physicsCore->CreateScene(sceneCreateInfo);

		auto& registry = m_entityScene.GetRegistry();

		// Rigid bodies
		{
			auto view = registry.view<const TagComponent, RigidbodyComponent>();
			view.each([&](const entt::entity id, const TagComponent& tag, RigidbodyComponent& rigidbody)
			{
				auto entity = m_entityScene.GetEntityFromHandle(id);
				CreateActorFromEntity(entity);
			});
		}

		// Character controller
		{
			auto view = registry.view<const TagComponent, CharacterControllerComponent>();
			view.each([&](const entt::entity id, const TagComponent& tag, CharacterControllerComponent& comp)
			{
				auto entity = m_entityScene.GetEntityFromHandle(id);

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

				auto controllerActor = m_physicsScene->CreateControllerActor(createInfo);

				comp.actorId = controllerActor->GetID();
				m_entityToPhysicsControllerActor[entity.GetID()] = comp.actorId;
			});
		}

		{
			const auto stats = m_physicsScene->GetStatistics();
			VT_LOGC(Trace, LogVoltPhysics, "Created Physics Scene with {} actors and {} controller actors.", stats.actorCount, stats.controllerActorCount);
		}
	}

	EntityPhysicsScene::~EntityPhysicsScene()
	{
		if (m_transformChangedCallbackID != 0)
		{
			m_entityScene.UnregisterTransformChangedCallback(m_transformChangedCallbackID);
		}

		if (m_entityDestroyedCallbackID != 0)
		{
			m_entityScene.UnregisterEntityDestroyedCallback(m_entityDestroyedCallbackID);
		}

		VT_LOGC(Trace, LogVoltPhysics, "Destroyed Physics Scene.");
	}

	void EntityPhysicsScene::Update(float deltaTime)
	{
		m_physicsScene->Simulate(deltaTime);
	}

	void EntityPhysicsScene::ExecuteRigidbodySystem()
	{
		VT_PROFILE_FUNCTION();

		auto& registry = m_entityScene.GetRegistry();

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_BodyTypeUpdated, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_BodyTypeUpdated&, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);

				m_physicsScene->RemoveActor(actorId);

				CreateActorFromEntity(m_entityScene.GetEntityFromHandle(id));

				registry.remove<Internal::RigidbodyComponentInternal_BodyTypeUpdated>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_LayerIdUpdated, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_LayerIdUpdated& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->AssignToPhysicsLayer(data.layerId);
				registry.remove<Internal::RigidbodyComponentInternal_LayerIdUpdated>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_MassUpdated, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_MassUpdated& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetMass(data.mass);
				registry.remove<Internal::RigidbodyComponentInternal_MassUpdated>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_LinearDragUpdated, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_LinearDragUpdated& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetLinearDrag(data.linearDrag);
				registry.remove<Internal::RigidbodyComponentInternal_LinearDragUpdated>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_AngularDragUpdated, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_AngularDragUpdated& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetAngularDrag(data.angularDrag);
				registry.remove<Internal::RigidbodyComponentInternal_AngularDragUpdated>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_LockFlagsUpdated, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_LockFlagsUpdated& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetLockFlags(static_cast<PhysicsActorLockFlags>(data.lockFlags));
				registry.remove<Internal::RigidbodyComponentInternal_LockFlagsUpdated>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_DisableGravityUpdated, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_DisableGravityUpdated& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetGravityDisabled(data.disableGravity);
				registry.remove<Internal::RigidbodyComponentInternal_DisableGravityUpdated>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_IsKinematicUpdated, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_IsKinematicUpdated& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetIsKinematic(data.isKinematic);
				registry.remove<Internal::RigidbodyComponentInternal_IsKinematicUpdated>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_SetKinematicTarget, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_SetKinematicTarget& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetKinematicTarget(data.translation, data.rotation);
				registry.remove<Internal::RigidbodyComponentInternal_SetKinematicTarget>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_SetLinearVelocity, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_SetLinearVelocity& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetLinearVelocity(data.velocity);
				registry.remove<Internal::RigidbodyComponentInternal_SetLinearVelocity>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_SetAngularVelocity, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_SetAngularVelocity& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetAngularVelocity(data.velocity);
				registry.remove<Internal::RigidbodyComponentInternal_SetAngularVelocity>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_SetMaxLinearVelocity, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_SetMaxLinearVelocity& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetMaxLinearVelocity(data.velocity);
				registry.remove<Internal::RigidbodyComponentInternal_SetMaxLinearVelocity>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_SetMaxAngularVelocity, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_SetMaxAngularVelocity& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->SetMaxAngularVelocity(data.velocity);
				registry.remove<Internal::RigidbodyComponentInternal_SetMaxAngularVelocity>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_AddForce_Force, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_AddForce_Force& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->AddForce(data.force, ForceMode::Force);
				registry.remove<Internal::RigidbodyComponentInternal_AddForce_Force>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_AddForce_Impulse, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_AddForce_Impulse& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->AddForce(data.force, ForceMode::Impulse);
				registry.remove<Internal::RigidbodyComponentInternal_AddForce_Impulse>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_AddForce_VelocityChange, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_AddForce_VelocityChange& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->AddForce(data.force, ForceMode::VelocityChange);
				registry.remove<Internal::RigidbodyComponentInternal_AddForce_VelocityChange>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_AddForce_Acceleration, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_AddForce_Acceleration& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->AddForce(data.force, ForceMode::Acceleration);
				registry.remove<Internal::RigidbodyComponentInternal_AddForce_Acceleration>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_AddTorque_Force, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_AddTorque_Force& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->AddTorque(data.torque, ForceMode::Force);
				registry.remove<Internal::RigidbodyComponentInternal_AddTorque_Force>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_AddTorque_Impulse, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_AddTorque_Impulse& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->AddTorque(data.torque, ForceMode::Impulse);
				registry.remove<Internal::RigidbodyComponentInternal_AddTorque_Impulse>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_AddTorque_VelocityChange, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_AddTorque_VelocityChange& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->AddTorque(data.torque, ForceMode::VelocityChange);
				registry.remove<Internal::RigidbodyComponentInternal_AddTorque_VelocityChange>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_AddTorque_Acceleration, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_AddTorque_Acceleration& data, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->AddTorque(data.torque, ForceMode::Acceleration);
				registry.remove<Internal::RigidbodyComponentInternal_AddTorque_Acceleration>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_WakeUp, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_WakeUp, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->Wake();
				registry.remove<Internal::RigidbodyComponentInternal_WakeUp>(id);
			});
		}

		{
			auto view = registry.view<const Internal::RigidbodyComponentInternal_PutToSleep, IDComponent>();
			view.each([&](const entt::entity id, const Internal::RigidbodyComponentInternal_PutToSleep, const IDComponent& idComp)
			{
				auto actorId = m_entityToPhysicsActor.at(idComp.id);
				auto actor = m_physicsScene->GetActor(actorId);

				actor->PutToSleep();
				registry.remove<Internal::RigidbodyComponentInternal_PutToSleep>(id);
			});
		}
	}

	void EntityPhysicsScene::CreateActorFromEntity(Entity entity)
	{
		auto physicsCore = SubSystemManager::GetSubSystem<PhysicsSubSystem>()->GetPhysicsCore();

		auto& rigidbody = entity.GetComponent<RigidbodyComponent>();

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
		createInfo.debugName = entity.GetTag();

		auto physicsActor = m_physicsScene->CreateActor(createInfo);

		rigidbody.actorId = physicsActor->GetID();
		m_physicsActorToEntity[physicsActor->GetID()] = entity.GetID();
		m_entityToPhysicsActor[entity.GetID()] = physicsActor->GetID();

		if (entity.HasComponent<BoxColliderComponent>())
		{
			auto& boxComp = entity.GetComponent<BoxColliderComponent>();

			BoxColliderCreateInfo colliderCreateInfo{};
			colliderCreateInfo.halfSize = boxComp.halfSize;
			colliderCreateInfo.isTrigger = boxComp.isTrigger;
			colliderCreateInfo.offset = boxComp.offset * entity.GetScale();
			colliderCreateInfo.scale = entity.GetScale();
			colliderCreateInfo.targetActor = physicsActor.GetRaw();
			colliderCreateInfo.physicalMaterial = physicsCore->CreateMaterial({});

			boxComp.colliderId = physicsActor->AddCollider(colliderCreateInfo);
		}

		if (entity.HasComponent<SphereColliderComponent>())
		{
			auto& sphereComp = entity.GetComponent<SphereColliderComponent>();

			SphereColliderCreateInfo colliderCreateInfo{};
			colliderCreateInfo.radius = sphereComp.radius;
			colliderCreateInfo.isTrigger = sphereComp.isTrigger;
			colliderCreateInfo.offset = sphereComp.offset * entity.GetScale();
			colliderCreateInfo.scale = entity.GetScale();
			colliderCreateInfo.targetActor = physicsActor.GetRaw();
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
			colliderCreateInfo.offset = capsuleComp.offset * entity.GetScale();
			colliderCreateInfo.scale = entity.GetScale();
			colliderCreateInfo.targetActor = physicsActor.GetRaw();
			colliderCreateInfo.physicalMaterial = physicsCore->CreateMaterial({});

			capsuleComp.colliderId = physicsActor->AddCollider(colliderCreateInfo);
		}
	}
}
