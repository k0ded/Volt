#include "vspch.h"

#include "Volt-Scene/Scene.h"
#include "Volt-Scene/EntityDescriptionSerializer.h"
#include "Volt-Scene/EntityDescription.h"
#include "Volt-Scene/EntityDescCustomMetadata.h"
#include "Volt-Scene/EntityUtility.h"

#include <Volt-Physics/RigidbodyComponent.h>
#include <Volt-Physics/EntityPhysicsScene.h>

#include <Volt-Animation/AnimationManager.h>

#include <Volt-Core/Algorithms.h>

#include <Volt-CoreComponents/LightComponents.h>
#include <Volt-CoreComponents/RenderingComponents.h>
#include <Volt-Renderer/RenderScene.h>
#include <Volt-Renderer/Camera/Camera.h>

#include <SubSystem/SubSystemManager.h>

#include <EntitySystem/Entity.h>
#include <EntitySystem/Scripting/CommonComponent.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetSerializerRegistry.h>

#include <CoreUtilities/Math/Math.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/FileIO/YAMLMemoryStreamReader.h>

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

	void Scene::UpdateEditor(float aDeltaTime)
	{
		VT_PROFILE_FUNCTION();

		m_statistics.entityCount = m_entityScene.GetEntityAliveCount();

		ForEachWithComponents<CameraComponent, const TransformComponent>([&](entt::entity id, CameraComponent& cameraComp, const TransformComponent& transComp)
		{
			if (transComp.visible)
			{
				Entity entity = { id, &m_entityScene };

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

	void Scene::LoadEntities()
	{
		if (m_isFinishedLoadingEntities)
		{
			return;
		}
		VT_PROFILE_MESSAGE("START LOADING ENTITIES");
		JobRef job = JobSystem::CreateJob("Register Entities", ExecutionPriority::Latent, [this]()
		{
			//collect all entity descriptions to spawn
			Vector<AssetHandle> allEntityDescAssetsForScene = Volt::AssetManager::GetAllAssetsOfType<Volt::EntityDesc>();
			for (const AssetHandle& handle : allEntityDescAssetsForScene)
			{
				const AssetMetadata meta = Volt::AssetManager::GetMetadataFromHandle(handle);
				const EntityDescCustomMetadata& customMeta = meta.GetCustomData<EntityDescCustomMetadata>();

				if (customMeta.sceneHandle != this->handle)
				{
					continue;
				}
				m_entityIDToDescHandle.emplace(customMeta.entityID, handle);
			}


			TaskGraph taskGraph{ ExecutionPriority::Latent };
			TaskGraph::Task* createEntitiesTask = taskGraph.AddTask("Create Entities", [this]()
			{
				entt::registry& registry = m_entityScene.GetRegistry();
				registry.reserve(m_entityIDToDescHandle.size());
				for (const auto& [entityID, descHandle] : m_entityIDToDescHandle)
				{
					m_entityScene.CreateEntityWithID(entityID);
				}
			});

			//parse entity descriptions to YAML
			Vector<TaskGraph::Task*> parseEntityDescTasks;
			parseEntityDescTasks.reserve(m_entityIDToDescHandle.size());

			//keep track of all the parsed yamlReaders
			Ref<std::unordered_map<EntityID, Ref<YAMLMemoryStreamReader>>> yamlReaders = CreateRef<std::unordered_map<EntityID, Ref<YAMLMemoryStreamReader>>>();
			yamlReaders->reserve(m_entityIDToDescHandle.size());

			//keep track of what components each entity needs
			Ref<Map<EntityID, Vector<VoltGUID>>> entityToComponentTypes = CreateRef<Map<EntityID, Vector<VoltGUID>>>();
			entityToComponentTypes->reserve(m_entityIDToDescHandle.size());
			for (const auto& [entityID, descHandle] : m_entityIDToDescHandle)
			{
				//create the reader for this entity
				Ref<YAMLMemoryStreamReader> reader = CreateRef<YAMLMemoryStreamReader>();
				yamlReaders->insert({ entityID,reader });

				//create the entry for this entity
				entityToComponentTypes->insert({ entityID,  {} });

				parseEntityDescTasks.push_back(taskGraph.AddTask("Parse EntityDesc Data", [descHandle, entityID, reader, entityToComponentTypes]
				{
					Ref<EntityDesc> entityDesc = AssetManager::GetAsset<EntityDesc>(descHandle);

					reader->ReadBuffer(entityDesc->GetEntitySpawnData());

					//todo_fabian: we probably want to be able to have the
					// entity descriptions not loaded but the entity present...
					//for now DirtyAssetManager relies on the desc being loaded
					//AssetManager::Get().UnloadAsset(descHandle);

					entityToComponentTypes->at(entityID) = EntityDescSerializer::FindComponentTypes(*reader);
				}));
			}


			//arrange component types to map from type to entityIDs to quickly create them concurrently later
			Ref<Map<VoltGUID, Vector<EntityID>>> componentTypeToOwningEntities = CreateRef<Map<VoltGUID, Vector<EntityID>>>();
			TaskGraph::Task* arrangeComponentsToEntityIDTask = taskGraph.AddTaskWithDependencies("Arrange Components To EntityIDs", parseEntityDescTasks, [componentTypeToOwningEntities, entityToComponentTypes]()
			{
				VT_LOG(Warning, "Arranging components to entityIDs");
				for (const auto& [entityID, componentTypes] : *entityToComponentTypes)
				{
					for (VoltGUID componentType : componentTypes)
					{
						(*componentTypeToOwningEntities)[componentType].push_back(entityID);
					}
				}
			});

			//new task graph to be able to create a separate task per component type
			taskGraph.AddTaskWithDependencies("Launch Component Creation and Serialization Jobs", { arrangeComponentsToEntityIDTask, createEntitiesTask }, [this, componentTypeToOwningEntities, entityToComponentTypes, yamlReaders]()
			{
				VT_LOG(Warning, "Launch Component Creation and Serialization Jobs");

				TaskGraph componentTaskGraph{ ExecutionPriority::Latent };

				//create all components
				Vector<TaskGraph::Task*> createComponentsTasks;
				createComponentsTasks.reserve(componentTypeToOwningEntities->size());
				for (const auto& [componentType, entityIDs] : *componentTypeToOwningEntities)
				{
					//todo: there should be a different system for order of initialization so that we dont have to make a special case
					if (componentType == GetTypeGUID<IDComponent>() ||
					componentType == GetTypeGUID<TransformComponent>() ||
					componentType == GetTypeGUID<TagComponent>() ||
					componentType == GetTypeGUID<RelationshipComponent>() ||
					componentType == GetTypeGUID<CommonComponent>())
					{
						continue;
					}

					createComponentsTasks.push_back(componentTaskGraph.AddTask("Create Components", [this, componentType, componentTypeToOwningEntities]()
					{
						entt::registry& registry = m_entityScene.GetRegistry();

						for (EntityID entityID : componentTypeToOwningEntities->at(componentType))
						{
							entt::entity entityHandle = m_entityScene.GetEntityHandleFromID(entityID);

							ComponentRegistry::Helpers::AddComponentWithGUID(componentType, registry, entityHandle);
						}
					}
					));
				}

				//todo_fabian: this can be multiple jobs when some issues are fixed with the task graph
				TaskGraph::Task* deserializeComponentsTask = componentTaskGraph.AddTaskWithDependencies("Deserialize Entities Component Datas", createComponentsTasks, [this, entityToComponentTypes, yamlReaders]()
				{
					for (const auto& [entityID, componentTypes] : *entityToComponentTypes)
					{
						const EntityDescSerializer& serializer = reinterpret_cast<const EntityDescSerializer&>(AssetSerializerRegistry::Get().GetSerializer(AssetTypes::EntityDesc));
						Volt::Entity entity = m_entityScene.GetEntityFromID(entityID);
						YAMLMemoryStreamReader& reader = *yamlReaders->at(entityID);
						serializer.DeserializeEntityInPlace(entity, reader);
					}
				});

				//todo_fabian: this can be multiple jobs when some issues are fixed with the task graph
				//we initialize all components after all components have gotten 
				TaskGraph::Task* initializeComponentsTask = componentTaskGraph.AddTaskWithDependencies("Initialize All Components", { deserializeComponentsTask }, [this, entityToComponentTypes]()
				{
					for (const auto& [entityID, componentTypes] : *entityToComponentTypes)
					{
						for (const VoltGUID& componentType : componentTypes)
						{
							const ICommonTypeDesc* typeDesc = GetComponentRegistry().GetTypeDescFromGUID(componentType);
							const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);
							componentDesc->OnInitialize(m_entityScene.GetEntityFromID(entityID));
						}
					}
				});

				componentTaskGraph.AddTaskWithDependencies("Finished loading entities", { initializeComponentsTask }, [this]()
				{
					VT_PROFILE_MESSAGE("FINISH LOADING ENTITIES");
					m_isFinishedLoadingEntities = true;
				});

				componentTaskGraph.Execute();
			});

			taskGraph.Execute();
		});
		JobSystem::RunJob(job);
	}

	void Scene::UnloadEntities()
	{
		//todo_fabian: we probably want to be able to have the
		// entity descriptions not loaded but the entity present...
		//for now DirtyAssetManager relies on the desc being loaded
		for (const auto& [id, descHandle] : m_entityIDToDescHandle)
		{
			AssetManager::Get().UnloadAsset(descHandle);
		}
		Clear();

	}

	Entity Scene::CreateEntity(const std::string& tag)
	{
		Entity newEntity = m_entityScene.CreateEntity(tag);
		VT_ENSURE(newEntity);

		//todo: World Engine
		//m_worldEngine.AddEntity(newEntity);

		CreateEntityDescForEntity(newEntity.GetID());

		return newEntity;
	}

	Entity Scene::CreateEntityWithID(const EntityID& id)
	{
		Entity newEntity = m_entityScene.CreateEntityWithID(id);
		VT_ENSURE(newEntity);

		//todo: World Engine
		//m_worldEngine.AddEntity(newEntity);

		return newEntity;
	}

	Volt::AssetHandle Scene::CreateEntityDescForEntity(const EntityID& id)
	{
		VT_ENSURE(!m_entityIDToDescHandle.contains(id));
		VT_ENSURE(m_entityScene.IsEntityValid(id));

		std::string name = std::to_string(id);
		Ref<Volt::EntityDesc> asset;
		if (Volt::AssetManager::IsMemoryAsset(this->handle))
		{
			asset = Volt::AssetManager::CreateMemoryAsset<Volt::EntityDesc>(name, id, this->handle);
		}
		else
		{
			asset = Volt::AssetManager::CreateAsset<Volt::EntityDesc>(name, id, this->handle);
		}
		m_entityIDToDescHandle.emplace(id, asset->handle);
		return asset->handle;
	}

	Entity Scene::GetEntityFromID(const EntityID id) const
	{
		return m_entityScene.GetEntityFromID(id);
	}

	Entity Scene::GetEntityFromHandle(entt::entity entityHandle) const
	{
		return  m_entityScene.GetEntityFromHandle(entityHandle);
	}

	Volt::AssetHandle Scene::GetEntityDescHandleFromEntityID(EntityID entityID) const
	{
		if (!m_entityIDToDescHandle.contains(entityID))
		{
			return Volt::Asset::Null();
		}
		return m_entityIDToDescHandle.at(entityID);
	}

	void Scene::DestroyEntity(Entity entity)
	{
		DestroyEntity(entity, nullptr, nullptr);
	}

	void Scene::DestroyEntity(Entity entity, Vector<EntityID>& outDestroyedEntities)
	{
		DestroyEntity(entity, &outDestroyedEntities, nullptr);
	}

	void Scene::DestroyEntity(Entity entity, Vector<AssetHandle>& outDestroyedEntityDescs)
	{
		DestroyEntity(entity, nullptr, &outDestroyedEntityDescs);
	}

	void Scene::DestroyEntity(Entity entity, Vector<EntityID>* outDestroyedEntities, Vector<Volt::AssetHandle>* outDestroyedEntityDescs)
	{
		Vector<EntityID> destroyedEntities;
		m_entityScene.DestroyEntity(entity.GetID(), &destroyedEntities);

		for (EntityID destroyedEnt : destroyedEntities)
		{
			if (outDestroyedEntityDescs)
			{
				outDestroyedEntityDescs->push_back(m_entityIDToDescHandle[destroyedEnt]);
			}
			m_entityIDToDescHandle.erase(destroyedEnt);
		}

		if (outDestroyedEntities)
		{
			*outDestroyedEntities = destroyedEntities;
		}
	}

	void Scene::InvalidateEntityTransform(const EntityID& entityId)
	{
		Vector<EntityID> invalidatedEntities = m_entityScene.InvalidateEntityTransform(entityId);

		//todo: World Engine
		/*if (m_sceneSettings.useWorldEngine)
		{
			Vector<EntityID> invalidatedEntities = m_entityScene.InvalidateEntityTransform(entityId);

			for (const auto& id : invalidatedEntities)
			{
				Entity currentEntity = GetEntityFromID(id);
				m_worldEngine.OnEntityMoved(currentEntity);
			}
		}*/
	}

	bool Scene::IsEntityValid(EntityID entityId) const
	{
		return m_entityScene.IsEntityValid(entityId);
	}

	Ref<Scene> Scene::CreateDefaultScene(const std::string& name, bool createDefaultMesh, bool asMemoryAsset)
	{
		Ref<Scene> newScene;
		if (asMemoryAsset)
		{
			newScene = Volt::AssetManager::CreateMemoryAsset<Scene>(name);
		}
		else
		{
			newScene = Volt::AssetManager::CreateAsset<Scene>(name);
		}

		// Setup
		{
			// Cube
			if (createDefaultMesh)
			{
				auto ent = newScene->CreateEntity("Cube");

				auto& meshComp = ent.AddComponent<MeshComponent>();
				meshComp.handle = AssetManager::GetAssetHandleFromFilePath("Engine/Meshes/Primitives/SM_Cube.vtasset");
				MeshComponent::OnMemberChanged(MeshComponent::MeshEntity(ent));
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
				SkylightComponent::OnMemberChanged(SkylightComponent::LightEntity(ent));
			}

			// Camera
			{
				auto ent = newScene->CreateEntity("Camera");
				ent.AddComponent<CameraComponent>();

				ent.SetPosition({ 0.f, 0.f, -500.f });
			}
		}

		newScene->m_sceneSettings.useWorldEngine = true;
		newScene->m_isFinishedLoadingEntities = true;

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

			auto entity = otherScene->CreateEntityWithID(uuid);
			CopyEntity(Entity{ id, &m_entityScene }, entity);

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
		const TQS entityWorldTQS = m_entityScene.GetEntityWorldTQS(entity);

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

	Vector<Entity> Scene::GetAllEntities() const
	{
		const auto& registry = m_entityScene.GetRegistry();

		Vector<Entity> result{};
		result.reserve(registry.alive());

		registry.each([&](const entt::entity id)
		{
			result.emplace_back(Entity{ id, &m_entityScene });
		});

		return result;
	}

	TQS Scene::GetEntityWorldTQS(const Entity& entity) const
	{
		return m_entityScene.GetEntityWorldTQS(entity);
	}
}
