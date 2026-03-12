#include "vspch.h"

#include "Volt-Scene/Scene.h"
#include "Volt-Scene/EntityDescSerialization.h"
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

#include <CoreUtilities/Math/Math.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::Scene, Scene);

	Scene::Scene(const SceneInitializer& sceneInitializer)
		: m_sceneInitializer(sceneInitializer),
		m_sceneExtensionManager(*this)
	{
		Initialize();
	}

	Scene::Scene()
		: m_sceneExtensionManager(*this)
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

		bool apa =  m_entityScene.IsEntityValid(EntityID::Null());
		apa;
		JobRef job = JobSystem::CreateJob("Register Entities", ExecutionPriority::Latent, [this]()
		{
			//collect all entity descriptions to spawn
			AssetRegistryIteratorFilter iteratorFilter;
			iteratorFilter.AddAssetType<Volt::EntityDesc>();

			g_assetManager->IterateAssetRegistryWithFilter(iteratorFilter, [this](ReadOnlyAssetMetadata assetMetadata)
			{
				const EntityDescCustomMetadata& customMeta = assetMetadata->GetCustomData<EntityDescCustomMetadata>();

				if (customMeta.sceneHandle != this->GetAssetHandle())
				{
					return true;
				}

				m_entityIDToDescHandle.emplace(customMeta.entityID, assetMetadata->handle);

				return true;
			});

			TaskGraph taskGraph{ ExecutionPriority::Latent };
			TaskGraph::Task* createEntitiesTask = taskGraph.AddTask("Create Entities", [this]()
			{
				entt::registry& registry = m_entityScene.GetRegistry();
				registry.reserve(m_entityIDToDescHandle.size());
				for (const auto& [entityID, descHandle] : m_entityIDToDescHandle)
				{
					m_entityScene.CreateEntityWithNoComponentsForID(entityID);
				}
			});

			using EntityComponentDataMap = Map<EntityID, const EntityDescSerialization::ComponentData*>;
			using ComponentTypeToOwningEntitiesMap = Map<VoltGUID, Vector<EntityID>>;
			using EntityToComponentTypesMap = Map<EntityID, Vector<VoltGUID>>;

			//parse entity descriptions to YAML
			Vector<TaskGraph::Task*> parseEntityDescTasks;
			parseEntityDescTasks.reserve(m_entityIDToDescHandle.size());

			struct EntitySerializationTaskGraphData
			{
				EntityComponentDataMap entityComponentDataMap;
				EntityToComponentTypesMap entityToComponentTypes;
				ComponentTypeToOwningEntitiesMap componentTypeToOwningEntities;
				Vector<AssetReference<EntityDesc>> loadedEntityDescs;
			};

			EntitySerializationTaskGraphData* taskGraphData = new EntitySerializationTaskGraphData;

			//keep track of all the parsed yamlReaders
			taskGraphData->entityComponentDataMap.reserve(m_entityIDToDescHandle.size());

			//keep track of what components each entity needs
			taskGraphData->entityToComponentTypes.reserve(m_entityIDToDescHandle.size());

			// Keep entity desc assets alive during loading
			taskGraphData->loadedEntityDescs.resize(m_entityIDToDescHandle.size());

			for (uint32_t index = 0; const auto& [entityID, descHandle] : m_entityIDToDescHandle)
			{
				//create the reader for this entity
				taskGraphData->entityComponentDataMap.insert({ entityID, nullptr });

				//create the entry for this entity
				taskGraphData->entityToComponentTypes.insert({ entityID,  {} });

				parseEntityDescTasks.push_back(taskGraph.AddTask("Parse EntityDesc Data", [descHandle, entityID, taskGraphData, index]
				{
					AssetReference<EntityDesc> entityDesc;
					if (g_assetManager->TryGetAssetImmediately(descHandle, entityDesc))
					{
						taskGraphData->loadedEntityDescs[index] = entityDesc;

						taskGraphData->entityComponentDataMap.at(entityID) = &entityDesc->GetComponentData();

						for (const EntityDescSerialization::ComponentHeader& componentHeader : entityDesc->GetComponentData().headers)
						{
							taskGraphData->entityToComponentTypes.at(entityID).emplace_back(componentHeader.componentGUID);
						}
					}
				}));

				index++;
			}


			//arrange component types to map from type to entityIDs to quickly create them concurrently later
			TaskGraph::Task* arrangeComponentsToEntityIDTask = taskGraph.AddTaskWithDependencies("Arrange Components To EntityIDs", parseEntityDescTasks, [taskGraphData]()
			{
				for (const auto& [entityID, componentTypes] : taskGraphData->entityToComponentTypes)
				{
					for (VoltGUID componentType : componentTypes)
					{
						taskGraphData->componentTypeToOwningEntities[componentType].push_back(entityID);
					}
				}
			});

			//new task graph to be able to create a separate task per component type
			taskGraph.AddTaskWithDependencies("Launch Component Creation and Serialization Jobs", { arrangeComponentsToEntityIDTask, createEntitiesTask }, [this, taskGraphData]()
			{
				TaskGraph componentTaskGraph{ ExecutionPriority::Latent };

				//create all components
				Vector<TaskGraph::Task*> createComponentsTasks;
				createComponentsTasks.reserve(taskGraphData->componentTypeToOwningEntities.size());
				for (const auto& [componentType, entityIDs] : taskGraphData->componentTypeToOwningEntities)
				{
					createComponentsTasks.push_back(componentTaskGraph.AddTask("Create Components", [this, componentType, taskGraphData]()
					{
						entt::registry& registry = m_entityScene.GetRegistry();

						for (EntityID entityID : taskGraphData->componentTypeToOwningEntities.at(componentType))
						{
							entt::entity entityHandle = m_entityScene.GetEntityHandleFromID(entityID);

							ComponentRegistry::Helpers::AddComponentWithGUID(componentType, registry, entityHandle);
						}
					}	
					));
				}

				//todo_fabian: this can be multiple jobs when some issues are fixed with the task graph
				TaskGraph::Task* deserializeComponentsTask = componentTaskGraph.AddTaskWithDependencies("Deserialize Entities Component Datas", createComponentsTasks, [this, taskGraphData]()
				{
					for (const auto& [entityID, componentTypes] : taskGraphData->entityToComponentTypes)
					{
						const EntityDescSerialization::ComponentData* componentData = taskGraphData->entityComponentDataMap.at(entityID);

						Volt::Entity entity = m_entityScene.GetEntityFromID(entityID);
						//since the component data doesnt have the entity ID we have to set it manually here
						entity.GetComponent<IDComponent>().id = entityID;

						EntityDescSerialization::ApplyComponentData(entity, const_cast<EntityDescSerialization::ComponentData&>(*componentData));
					}
				});

				//todo_fabian: this can be multiple jobs when some issues are fixed with the task graph
				//we initialize all components after all components have gotten 
				TaskGraph::Task* initializeComponentsTask = componentTaskGraph.AddTaskWithDependencies("Initialize All Components", { deserializeComponentsTask }, [this, taskGraphData]()
				{
					for (const auto& [entityID, componentTypes] : taskGraphData->entityToComponentTypes)
					{
						for (const VoltGUID& componentType : componentTypes)
						{
							const ICommonTypeDesc* typeDesc = ComponentRegistry::Get().GetTypeDescFromGUID(componentType);
							const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);
							componentDesc->OnInitialize(m_entityScene.GetEntityFromID(entityID));
						}
					}
				});

				componentTaskGraph.AddTaskWithDependencies("Finished loading entities", { initializeComponentsTask }, [this, taskGraphData]()
				{
					VT_PROFILE_MESSAGE("FINISH LOADING ENTITIES");
					m_isFinishedLoadingEntities = true;

					delete taskGraphData;
				});

				componentTaskGraph.Execute();
			});

			taskGraph.Execute();
		});
		JobSystem::RunJob(job);
	}

	void Scene::UnloadEntities()
	{
		Clear();
	}

	Entity Scene::CreateEntity(const std::string& tag)
	{
		Entity newEntity = m_entityScene.CreateEntity(tag);
		VT_ENSURE(newEntity);

		CreateEntityDescForEntity(newEntity.GetID());
		m_sceneExtensionManager.OnEntityCreated(newEntity);

		return newEntity;
	}

	Entity Scene::CreateEntityWithID(const EntityID& id)
	{
		Entity newEntity = m_entityScene.CreateEntityWithID(id);
		VT_ENSURE(newEntity);

		CreateEntityDescForEntity(newEntity.GetID());
		m_sceneExtensionManager.OnEntityCreated(newEntity);

		return newEntity;
	}

	Entity Scene::CreateEntityWithIDForExistingDescription(const EntityID& id, Volt::AssetHandle existingEntityDescHandle)
	{
		ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(existingEntityDescHandle);
		VT_MAYBE_UNUSED const EntityDescCustomMetadata& customMeta = assetMetadata->GetCustomData<EntityDescCustomMetadata>();

		VT_ENSURE(customMeta.sceneHandle == this->GetAssetHandle());
		VT_ENSURE(customMeta.entityID == id);

		Entity newEntity = m_entityScene.CreateEntityWithID(id);
		VT_ENSURE(newEntity);

		m_entityIDToDescHandle.emplace(id, existingEntityDescHandle);
		m_sceneExtensionManager.OnEntityCreated(newEntity);

		return newEntity;
	}

	AssetHandle Scene::CreateEntityDescForEntity(const EntityID& id)
	{
		VT_ENSURE(!m_entityIDToDescHandle.contains(id));
		VT_ENSURE(m_entityScene.IsEntityValid(id));

		std::string name = std::to_string(id);
		
		AssetReference<EntityDesc> asset;
		if (this->IsFlagSet(AssetFlag::MemoryOnly))
		{
			asset = g_assetManager->CreateMemoryAsset<EntityDesc>(name, id, GetAssetHandle());
		}
		else
		{
			asset = g_assetManager->CreateAsset<EntityDesc>(name, id, GetAssetHandle());
		}

		m_entityIDToDescHandle.emplace(id, asset->GetAssetHandle());
		m_createdEntityDescs.emplace_back(asset);

		return asset->GetAssetHandle();
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

	void Scene::DestroyEntity(Entity entity, bool ignoreChildren)
	{
		DestroyEntity(entity, nullptr, nullptr, ignoreChildren);
	}

	void Scene::DestroyEntity(Entity entity, Vector<EntityID>& outDestroyedEntities, bool ignoreChildren)
	{
		DestroyEntity(entity, &outDestroyedEntities, nullptr, ignoreChildren);
	}

	void Scene::DestroyEntity(Entity entity, Vector<AssetHandle>& outDestroyedEntityDescs, bool ignoreChildren)
	{
		DestroyEntity(entity, nullptr, &outDestroyedEntityDescs, ignoreChildren);
	}

	void Scene::DestroyEntity(Entity entity, Vector<EntityID>* outDestroyedEntities, Vector<Volt::AssetHandle>* outDestroyedEntityDescs, bool ignoreChildren)
	{
		Vector<EntityID> destroyedEntities;
		m_entityScene.DestroyEntity(entity.GetID(), &destroyedEntities, false, ignoreChildren);

		for (EntityID destroyedEnt : destroyedEntities)
		{
			if (outDestroyedEntityDescs)
			{
				outDestroyedEntityDescs->push_back(m_entityIDToDescHandle[destroyedEnt]);
			}

			m_sceneExtensionManager.OnEntityDestroyed(destroyedEnt);
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

	AssetReference<Scene> Scene::CreateDefaultScene(const std::string& name, bool createDefaultMesh, bool asMemoryAsset)
	{
		AssetReference<Scene> newScene;
		if (asMemoryAsset)
		{
			newScene = g_assetManager->CreateMemoryAsset<Scene>(name);
		}
		else
		{
			newScene = g_assetManager->CreateAsset<Scene>(name);
		}

		// Setup
		{
			// Cube
			if (createDefaultMesh)
			{
				auto ent = newScene->CreateEntity("Cube");

				auto& meshComp = ent.AddComponent<MeshComponent>();
				meshComp.SetMesh(g_assetManager->GetAssetHandleFromFilepath("Engine/Meshes/Primitives/SM_Cube.vtasset"), ent.GetID());
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
				skyComp.environmentTextureHandle = g_assetManager->GetAssetHandleFromFilepath("Engine/Textures/HDRIs/defaultHDRI.vtasset");
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

	void Scene::Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata)
	{
		archive << m_sceneInitializer.name;
		archive << m_sceneSettings.useWorldEngine;
	}

	void Scene::CopyEntitiesTo(AssetReference<Scene> otherScene)
	{
		VT_PROFILE_FUNCTION();
	
		otherScene->Clear();

		auto& registry = m_entityScene.GetRegistry();
		registry.each([&](entt::entity id)
		{
			const EntityID uuid = registry.get<IDComponent>(id).id;

			auto entity = otherScene->CreateEntityWithID(uuid);
			CopyEntity(Entity{ id, &m_entityScene }, entity);
			entity.InitializeComponents();
			otherScene->InvalidateEntityTransform(entity.GetID());
			otherScene->GetWorldEngineMutable().OnEntityMoved(entity);
		});

		otherScene->m_isFinishedLoadingEntities = true;
	}

	void Scene::Clear()
	{
		m_createdEntityDescs.clear();
		m_entityIDToDescHandle.clear();
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
		if (m_sceneInitializer.shouldHaveRenderScene)
		{
			m_renderScene = CreateRef<RenderScene>(&m_entityScene);
			m_entityScene.SetRenderScene(m_renderScene.get());
		}

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
