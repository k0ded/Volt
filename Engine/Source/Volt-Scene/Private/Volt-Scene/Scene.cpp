#include "vspch.h"

#include "Volt-Scene/Scene.h"
#include "Volt-Scene/Entity.h"

#include <Volt-Physics/RigidbodyComponent.h>
#include <Volt-Physics/EntityPhysicsScene.h>

#include <Volt-Animation/AnimationManager.h>

#include <Volt-CoreComponents/LightComponents.h>
#include <Volt-CoreComponents/RenderingComponents.h>
#include <Volt-Renderer/RenderScene.h>
#include <Volt-Renderer/Camera/Camera.h>

#include <SubSystem/SubSystemManager.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetFactory.h>

#include <CoreUtilities/Math/Math.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::Scene, Scene);

	Scene::Scene(const std::string& name)
		: m_name(name)
	{
		Initialize();
	}

	Scene::Scene()
	{
		Initialize();
	}

	Scene::~Scene()
	{
		m_entityScene.ClearScene();
		m_renderScene = nullptr;
	}

	void Scene::SetRenderSize(uint32_t aWidth, uint32_t aHeight)
	{
		m_viewportWidth = aWidth;
		m_viewportHeight = aHeight;
	}

	void Scene::OnRuntimeStart()
	{
		CreatePhysicsScene();
		AnimationManager::Reset();

		m_isPlaying = true;
		m_timeSinceStart = 0.f;

		m_entityScene.OnRuntimeStart();
	}

	void Scene::OnRuntimeEnd()
	{
		m_entityScene.OnRuntimeEnd();
		m_isPlaying = false;

		m_entityPhysicsScene = nullptr;
	}

	void Scene::OnSimulationStart()
	{
		CreatePhysicsScene();
	}

	void Scene::OnSimulationEnd()
	{
		m_entityPhysicsScene = nullptr;
	}

	void Scene::Update(float aDeltaTime)
	{
		VT_PROFILE_FUNCTION();
		m_statistics.entityCount = m_entityScene.GetEntityAliveCount();
		
		m_entityScene.Update(aDeltaTime);

		AnimationManager::Update(aDeltaTime);
		m_entityPhysicsScene->Update(aDeltaTime);
		//m_visionSystem->Update(aDeltaTime); // #TODO_Scene

		m_timeSinceStart += aDeltaTime;
		m_currentDeltaTime = aDeltaTime;
	}

	void Scene::FixedUpdate(float aDeltaTime)
	{
		m_entityScene.FixedUpdate(aDeltaTime);
	}

	void Scene::UpdateEditor(float aDeltaTime)
	{
		VT_PROFILE_FUNCTION();

		m_statistics.entityCount = m_entityScene.GetEntityAliveCount();

		ForEachWithComponents<CameraComponent, const TransformComponent>([&](entt::entity id, CameraComponent& cameraComp, const TransformComponent& transComp)
		{
			if (transComp.visible)
			{
				Entity entity = { id, this };

				if (!cameraComp.camera)
				{
					return;
				}

				cameraComp.camera->SetPerspectiveProjection(cameraComp.fieldOfView, (float)m_viewportWidth / (float)m_viewportHeight, cameraComp.nearPlane, cameraComp.farPlane);
				cameraComp.camera->SetPosition(entity.GetPosition());
				cameraComp.camera->SetRotation(glm::eulerAngles(entity.GetRotation()));
			}
		});
	}

	void Scene::UpdateSimulation(float aDeltaTime)
	{
		m_entityPhysicsScene->Update(aDeltaTime);
		m_statistics.entityCount = m_entityScene.GetEntityAliveCount();
	}

	void Scene::SortScene()
	{
		m_entityScene.SortScene();
	}

	Entity Scene::CreateEntity(const std::string& tag)
	{
		EntityHelper newHelper = m_entityScene.CreateEntity(tag);
		VT_ENSURE(newHelper);

		Entity newEntity(newHelper.GetHandle(), this);
		m_worldEngine.AddEntity(newEntity);

		return newEntity;
	}

	Entity Scene::CreateEntityWithID(const EntityID& id, const std::string& tag)
	{
		EntityHelper newHelper = m_entityScene.CreateEntityWithID(id, tag);
		VT_ENSURE(newHelper);

		Entity newEntity(newHelper.GetHandle(), this);
		m_worldEngine.AddEntity(newEntity);

		return newEntity;
	}

	Entity Scene::GetEntityFromID(const EntityID id) const
	{
		EntityHelper helper = m_entityScene.GetEntityHelperFromEntityID(id);
		return Entity(helper.GetHandle(), const_cast<Scene*>(this));
	}

	Entity Scene::GetEntityFromHandle(entt::entity entityHandle) const
	{
		EntityHelper helper = m_entityScene.GetEntityHelperFromEntityHandle(entityHandle);
		return Entity(helper.GetHandle(), const_cast<Scene*>(this));
	}

	EntityHelper Scene::GetEntityHelperFromEntityID(EntityID entityId) const
	{
		return m_entityScene.GetEntityHelperFromEntityID(entityId);
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_entityScene.DestroyEntity(entity.GetID());
		SortScene();
	}

	void Scene::ParentEntity(Entity parent, Entity child)
	{
		if (!parent.IsValid() || !child.IsValid() || parent == child)
		{
			return;
		}

		// Check that it's not in the child chain
		bool isChild = false;
		IsRecursiveChildOf(parent, child, isChild);
		if (isChild)
		{
			return;
		}

		if (child.GetParent())
		{
			UnparentEntity(child);
		}

		auto& childChildren = child.GetComponent<RelationshipComponent>().children;

		if (auto it = std::find(childChildren.begin(), childChildren.end(), parent.GetID()) != childChildren.end())
		{
			return;
		}

		child.GetComponent<RelationshipComponent>().parent = parent.GetID();
		parent.GetComponent<RelationshipComponent>().children.emplace_back(child.GetID());

		ConvertToLocalSpace(child);
	}

	void Scene::UnparentEntity(Entity entity)
	{
		if (!entity.IsValid()) { return; }

		auto parent = entity.GetParent();
		if (!parent.IsValid())
		{
			return;
		}

		auto& children = parent.GetComponent<RelationshipComponent>().children;

		auto it = std::find(children.begin(), children.end(), entity.GetID());
		if (it != children.end())
		{
			children.erase(it);
		}

		//we need to convert to world space before removing the parent because it takes the parent transform into account
		ConvertToWorldSpace(entity);
		entity.GetComponent<RelationshipComponent>().parent = Entity::NullID();

		//we have to invalidate the transform here even though ConvertToWorldSpace already does it since it takes the parent into account
		InvalidateEntityTransform(entity.GetID());
	}

	void Scene::InvalidateEntityTransform(const EntityID& entityId)
	{
		Vector<EntityID> invalidatedEntities = m_entityScene.InvalidateEntityTransform(entityId);

		for (const auto& id : invalidatedEntities)
		{
			Entity currentEntity = GetEntityFromID(id);

			if (m_sceneSettings.useWorldEngine)
			{
				m_worldEngine.OnEntityMoved(currentEntity);
			}
		}
	}

	bool Scene::IsEntityValid(EntityID entityId) const
	{
		return m_entityScene.IsEntityValid(entityId);
	}

	Ref<Scene> Scene::CreateDefaultScene(const std::string& name, bool createDefaultMesh)
	{
		Ref<Scene> newScene = CreateRef<Scene>(name);

		// Setup
		{
			// Cube
			if (createDefaultMesh)
			{
				auto ent = newScene->CreateEntity("Cube");

				auto& meshComp = ent.AddComponent<MeshComponent>();
				meshComp.handle = AssetManager::GetAssetHandleFromFilePath("Engine/Meshes/Primitives/SM_Cube.vtasset");
				Volt::MeshComponent::OnMemberChanged(Volt::MeshComponent::MeshEntity(ent.GetScene()->GetEntityHelperFromEntityID(ent.GetID())));
			}

			// Light
			{
				auto ent = newScene->CreateEntity();
				ent.SetTag("Directional Light");
				ent.AddComponent<DirectionalLightComponent>();

				ent.SetRotation(glm::quat{ glm::vec3{ glm::radians(120.f), 0.f, 0.f } });
			}

			// Skylight
			{
				auto ent = newScene->CreateEntity("Skylight");
				SkylightComponent& skyComp = ent.AddComponent<SkylightComponent>();
				skyComp.environmentTextureHandle = AssetManager::GetAssetHandleFromFilePath("Engine/Textures/HDRIs/defaultHDRI.vtasset");
				skyComp.UpdateSceneLightData(true);
			}

			// Camera
			{
				auto ent = newScene->CreateEntity("Camera");
				ent.AddComponent<CameraComponent>();

				ent.SetPosition({ 0.f, 0.f, -500.f });
			}
		}

		newScene->m_sceneSettings.useWorldEngine = true;

		return newScene;
	}

	void Scene::CopyTo(Ref<Scene> otherScene)
	{
		VT_PROFILE_FUNCTION();

		otherScene->m_name = m_name;
		//otherScene->m_environment = m_environment; // #TODO_Scene
		otherScene->handle = handle;

		auto& registry = m_entityScene.GetRegistry();

		registry.each([&](entt::entity id)
		{
			const EntityID uuid = registry.get<IDComponent>(id).id;

			auto entity =  otherScene->CreateEntityWithID(uuid);
			Entity::Copy(Entity{ id, this }, entity, EntityCopyFlags::None);

			otherScene->InvalidateEntityTransform(entity.GetID());
			otherScene->GetWorldEngineMutable().OnEntityMoved(entity);
		});
	} 

	void Scene::Clear()
	{
		m_entityScene.ClearScene();
	}

	glm::mat4 Scene::GetWorldTransform(Entity entity) const
	{
		const TQS entityWorldTQS = m_entityScene.GetEntityWorldTQS(m_entityScene.GetEntityHelperFromEntityID(entity.GetID()));

		const glm::mat4 transform = glm::translate(glm::mat4{ 1.f }, entityWorldTQS.translation)
			* glm::mat4_cast(entityWorldTQS.rotation)
			* glm::scale(glm::mat4{ 1.f }, entityWorldTQS.scale);

		return transform;
	}

	Vector<Entity> Scene::FlattenEntityHeirarchy(Entity entity)
	{
		Vector<Entity> result;
		result.emplace_back(entity);

		for (auto child : entity.GetChildren())
		{
			auto childResult = FlattenEntityHeirarchy(child);

			for (auto& childRes : childResult)
			{
				result.emplace_back(childRes);
			}
		}

		return result;
	}

	void Scene::Initialize()
	{
		//m_visionSystem = CreateRef<Vision>(this); // #TODO_Scene
		m_renderScene = CreateRef<RenderScene>(&m_entityScene);

		m_entityScene.SetRenderScene(m_renderScene.get());

		m_worldEngine.Reset(this, 16, 4);
	}

	void Scene::CreatePhysicsScene()
	{
		m_entityPhysicsScene = CreateScope<EntityPhysicsScene>(m_entityScene);
	}

	bool Scene::IsRelatedTo(Entity entity, Entity otherEntity)
	{
		const auto flatHeirarchy = FlattenEntityHeirarchy(entity);
		for (const auto& ent : flatHeirarchy)
		{
			if (ent.GetID() == otherEntity.GetID())
			{
				return true;
			}
		}

		return false;
	}

	void Scene::IsRecursiveChildOf(Entity parent, Entity currentEntity, bool& outChild)
	{
		if (currentEntity.HasComponent<RelationshipComponent>())
		{
			auto& relComp = currentEntity.GetComponent<RelationshipComponent>();
			for (const auto& childId : relComp.children)
			{
				Entity child = GetEntityFromID(childId);
				outChild |= (parent.GetID() == childId) && (childId != parent.GetID());

				IsRecursiveChildOf(parent, child, outChild);
			}
		}
	}

	void Scene::ConvertToWorldSpace(Entity entity)
	{
		Entity parent = entity.GetParent();

		if (!parent)
		{
			return;
		}

		auto& transform = entity.GetComponent<TransformComponent>();

		const glm::mat4 transformMatrix = GetWorldTransform(entity);

		glm::vec3 r;
		Math::Decompose(transformMatrix, transform.position, r, transform.scale);

		transform.rotation = glm::quat{ r };

		InvalidateEntityTransform(entity.GetID());
	}

	void Scene::ConvertToLocalSpace(Entity entity)
	{
		Entity parent = entity.GetParent();

		if (!parent)
		{
			return;
		}

		auto& transform = entity.GetComponent<TransformComponent>();
		const glm::mat4 parentTransform = GetWorldTransform(parent);
		const glm::mat4 localTransform = glm::inverse(parentTransform) * transform.GetTransform();

		glm::vec3 r;
		Math::Decompose(localTransform, transform.position, r, transform.scale);
		transform.rotation = glm::quat{ r };

		InvalidateEntityTransform(entity.GetID());
	}

	void Scene::MarkEntityAsEdited(const Entity& entity)
	{
		VT_ENSURE_MSG(entity.IsValid(), "Entity is not valid! Only valid entities can be marked as edited!");
		m_entityScene.MarkEntityAsEdited(m_entityScene.GetEntityHelperFromEntityID(entity.GetID()));
	}

	void Scene::ClearEditedEntities()
	{
		m_entityScene.ClearEditedEntities();
	}

	Vector<Entity> Scene::GetAllEntities() const
	{
		const auto& registry = m_entityScene.GetRegistry();

		Vector<Entity> result{};
		result.reserve(registry.alive());

		registry.each([&](const entt::entity id)
		{
			result.emplace_back(Entity{ id, const_cast<Scene*>(this) });
		});

		return result;
	}

	Vector<Entity> Scene::GetAllEditedEntities() const
	{
		Vector<Entity> entities;

		for (const auto& entity : m_entityScene.GetEditedEntities())
		{
			entities.push_back(GetEntityFromID(entity));
		}

		return entities;
	}

	Vector<EntityID> Scene::GetAllRemovedEntities() const
	{
		Vector<EntityID> entities;

		for (const auto& entity : m_entityScene.GetRemovedEntities())
		{
			entities.push_back(entity);
		}

		return entities;
	}

	TQS Scene::GetEntityWorldTQS(const Entity& entity) const
	{
		return m_entityScene.GetEntityWorldTQS(m_entityScene.GetEntityHelperFromEntityID(entity.GetID()));
	}
}
