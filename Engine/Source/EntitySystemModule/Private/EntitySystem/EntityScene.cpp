#include "espch.h"

#include "EntitySystem/Entity.h"
#include "EntitySystem/EntityScene.h"
#include "EntitySystem/SceneEvents.h"
#include "EntitySystem/ComponentRegistry.h"

#include "EntitySystem/Scripting/CoreComponents.h"
#include "EntitySystem/Scripting/CommonComponent.h"
#include "EntitySystem/Scripting/ECSSystemRegistry.h"
#include "EntitySystem/Scripting/ECSBuilder.h"
#include "EntitySystem/Scripting/ScriptingEngine.h"
#include "EntitySystem/Scripting/CoreEnvironments.h"

#include <EventSystem/EventSystem.h>

#include <CoreUtilities/Time/TimeUtility.h>
#include <CoreUtilities/Profiling/Profiling.h>

#include <ranges>


namespace Volt
{
	EntityScene::EntityScene()
	{
		Initialize();
	}

	EntityScene::~EntityScene()
	{
		m_ecsBuilder = nullptr;
		m_scriptingEngine = nullptr;
	}

	void EntityScene::OnRuntimeStart()
	{
		m_isPlaying = true;
		m_scriptingEngine->OnRuntimeStart();

		ComponentOnStart();

		OnSceneRuntimeStartEvent startEvent(*this);
		EventSystem::DispatchEvent(startEvent);
	}

	void EntityScene::OnRuntimeEnd()
	{
		ComponentOnStop();

		OnSceneRuntimeEndEvent endEvent(*this);
		EventSystem::DispatchEvent(endEvent);

		m_scriptingEngine->OnRuntimeEnd();
		m_isPlaying = false;
	}

	void EntityScene::Update(float deltaTime)
	{
		VT_PROFILE_FUNCTION();
		auto& variableUpdateEnv = m_scriptingEngine->GetECSEnvironmentOfType<env::VariableUpdate>();
		variableUpdateEnv.deltaTime = deltaTime;

		m_ecsBuilder->GetGameLoop(GameLoop::Variable).Execute(*this);
	}

	void EntityScene::FixedUpdate(float deltaTime)
	{
		VT_PROFILE_FUNCTION();
		auto& fixedUpdateEnv = m_scriptingEngine->GetECSEnvironmentOfType<env::FixedUpdate>();
		fixedUpdateEnv.deltaTime = deltaTime;

		m_ecsBuilder->GetGameLoop(GameLoop::Fixed).Execute(*this);
	}

	void EntityScene::SortScene()
	{
		VT_PROFILE_FUNCTION();

		m_registry.sort<CommonComponent>([&](entt::entity lhs, entt::entity rhs)
		{
			const auto& lhsComp = m_registry.get<CommonComponent>(lhs);
			const auto& rhsComp = m_registry.get<CommonComponent>(rhs);

			return lhsComp.timeCreatedID < rhsComp.timeCreatedID;
		});
	}

	void EntityScene::ClearScene()
	{
		m_registry.clear();
	}

	Entity EntityScene::CreateEntity(const String& tag)
	{
		entt::entity entityHandle = m_registry.create();

		Entity newHelper(entityHandle, this);

		// Setup default components
		{
			auto& transformComponent = newHelper.AddComponent<TransformComponent>();
			transformComponent.position = 0.f;
			transformComponent.rotation = glm::identity<glm::quat>();
			transformComponent.scale = 1.f;

			auto& tagComponent = newHelper.AddComponent<TagComponent>();
			if (tag.empty())
			{
				tagComponent.tag = "New Entity";
			}
			else
			{
				tagComponent.tag = tag;
			}

			auto& idComponent = newHelper.AddComponent<IDComponent>();
			while (m_entityRegistry.Contains(idComponent.id))
			{
				idComponent.id = {};
			}

			auto& commonComponent = newHelper.AddComponent<CommonComponent>();
			commonComponent.timeCreatedID = TimeUtility::GetTimeSinceEpoch();

			newHelper.AddComponent<RelationshipComponent>();
		}

		m_entityRegistry.AddEntity(newHelper.GetID(), entityHandle);

		InvalidateEntityTransform(newHelper.GetID());
		return newHelper;
	}

	Entity EntityScene::CreateEntityWithID(EntityID id)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE(!m_entityRegistry.Contains(id));

		entt::entity entityHandle = m_registry.create();

		Entity newHelper(entityHandle, this);

		// Setup default components
		{
			auto& transformComponent = newHelper.AddComponent<TransformComponent>();
			transformComponent.position = 0.f;
			transformComponent.rotation = glm::identity<glm::quat>();
			transformComponent.scale = 1.f;

			auto& tagComponent = newHelper.AddComponent<TagComponent>();
			tagComponent.tag = "New Entity";

			auto& idComponent = newHelper.AddComponent<IDComponent>();
			idComponent.id = id;

			auto& commonComponent = newHelper.AddComponent<CommonComponent>();
			commonComponent.timeCreatedID = TimeUtility::GetTimeSinceEpoch();

			newHelper.AddComponent<RelationshipComponent>();
		}

		m_entityRegistry.AddEntity(id, entityHandle);
		InvalidateEntityTransform(newHelper.GetID());

		return newHelper;
	}

	Entity EntityScene::CreateEntityWithNoComponentsForID(EntityID id)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE(!m_entityRegistry.Contains(id));

		entt::entity entityHandle = m_registry.create();
		m_entityRegistry.AddEntity(id, entityHandle);

		Entity newEntity(entityHandle, this);
		return newEntity;
	}

	void EntityScene::DestroyEntity(EntityID id, Vector<EntityID>* outDestroyedEntities, bool isDestroyingChildFromParent, bool ignoreChildren)
	{
		if (!IsEntityValid(id))
		{
			return;
		}

		Entity helper = GetEntityFromID(id);

		// We need to handle the entity's parent and children.
		VT_ENSURE(helper.HasComponent<RelationshipComponent>());
		auto* relationshipComponent = &helper.GetComponent<RelationshipComponent>();

		//remove ourselves from our parent's children
		helper.UnparentEntity();

		//even when ignoring children, we still need to remove ourselves as a parent from our children
		if (ignoreChildren)
		{
			for (int32_t i = 0; i < relationshipComponent->children.size(); ++i)
			{
				EntityID childID = relationshipComponent->children.at(i);
				Entity child = GetEntityFromID(childID);
				child.UnparentEntity();
			}
		}
		else
		{
			// We need to do this backwards, otherwise we will be pointing to invalid indices
			for (int32_t i = static_cast<int32_t>(relationshipComponent->children.size()) - 1; i >= 0; --i)
			{
				DestroyEntity(relationshipComponent->children.at(i), outDestroyedEntities, true);

				// This is required, because removing components from entt::registry might
				// invalidate pointers.
				relationshipComponent = &helper.GetComponent<RelationshipComponent>();
			}
		}

		if (outDestroyedEntities)
		{
			outDestroyedEntities->push_back(id);
		}
		m_registry.destroy(helper.GetHandle());
		m_entityRegistry.RemoveEntity(id, helper.GetHandle());
	}

	Vector<EntityID> EntityScene::InvalidateEntityTransform(EntityID entityId)
	{
		Vector<EntityID> invalidatedEntities;
		invalidatedEntities.reserve(10);

		Vector<EntityID> entityStack;
		entityStack.reserve(10);
		entityStack.push_back(entityId);

		while (!entityStack.empty())
		{
			const EntityID currentId = entityStack.back();
			entityStack.pop_back();

			Entity entityHelper(m_entityRegistry.GetHandleFromID(currentId), this);

			VT_ENSURE(entityHelper.HasComponent<RelationshipComponent>());

			auto& relationshipComponent = entityHelper.GetComponent<RelationshipComponent>();

			m_transformCache.InvalidateTransform(currentId);

			// Call on transform changed on all components on entity
			for (auto&& curr : m_registry.storage())
			{
				auto& storage = curr.second;
				
				std::string_view tempTypeName = storage.type().name();
				StringView typeName(tempTypeName.data(), tempTypeName.size());

				const ICommonTypeDesc* typeDesc = Volt::ComponentRegistry::Get().GetTypeDescFromName(typeName);
				if (!typeDesc)
				{
					continue;
				}

				if (typeDesc->GetValueType() != ValueType::Component)
				{
					continue;
				}

				const IComponentTypeDesc* compTypeDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);

				if (storage.contains(entityHelper.GetHandle()))
				{
					compTypeDesc->OnTransformChanged(entityHelper);
				}
			}

			for (const auto& child : relationshipComponent.children)
			{
				entityStack.push_back(child);
			}

			invalidatedEntities.emplace_back(currentId);
		}

		return invalidatedEntities;
	}

	UUID64 EntityScene::RegisterTransformChangedCallback(TransformChangedCallbackFunc&& callback)
	{
		UUID64 uuid{};
		m_transformChangedCallbacks[uuid] = std::move(callback);
		return uuid;
	}

	void EntityScene::UnregisterTransformChangedCallback(UUID64 id)
	{
		if (m_transformChangedCallbacks.contains(id))
		{
			m_transformChangedCallbacks.erase(id);
		}
		else
		{
			VT_LOGC(Warning, LogEntitySystem, "Trying to unregister callback with ID {}, but that ID is not registered!", id);
		}
	}

	UUID64 EntityScene::RegisterEntityDestroyedCallback(EntityDestroyedCallbackFunc&& callback)
	{
		UUID64 uuid{};
		m_entityDestroyedCallbacks[uuid] = std::move(callback);
		return uuid;
	}

	void EntityScene::UnregisterEntityDestroyedCallback(UUID64 id)
	{
		if (m_entityDestroyedCallbacks.contains(id))
		{
			m_entityDestroyedCallbacks.erase(id);
		}
		else
		{
			VT_LOGC(Warning, LogEntitySystem, "Trying to unregister callback with ID {}, but that ID is not registered!", id);
		}
	}

	bool EntityScene::IsEntityValid(EntityID entityId) const
	{
		return m_registry.valid(m_entityRegistry.GetHandleFromID(entityId));
	}


	TQS EntityScene::GetEntityWorldTQS(const Entity& entityHelper) const
	{
		VT_PROFILE_FUNCTION();

		TQS resultTransform{};
		if (m_transformCache.TryGetCachedTransform(entityHelper.GetID(), resultTransform))
		{
			return resultTransform;
		}

		Vector<Entity> hierarchy{};
		hierarchy.emplace_back(entityHelper);

		Entity currentEntity = entityHelper;
		while (currentEntity.HasParent())
		{
			auto parent = currentEntity.GetParent();
			hierarchy.emplace_back(parent);
			currentEntity = parent;
		}

		for (const auto& ent : std::ranges::reverse_view(hierarchy))
		{
			const auto& transformComp = m_registry.get<TransformComponent>(ent.GetHandle());

			resultTransform.translation = resultTransform.translation + resultTransform.rotation * transformComp.position;
			resultTransform.rotation = resultTransform.rotation * transformComp.rotation;
			resultTransform.scale = resultTransform.scale * transformComp.scale;
		}

		m_transformCache.CacheTransform(entityHelper.GetID(), resultTransform);
		return resultTransform;
	}

	Entity EntityScene::GetEntityFromID(EntityID entityId) const
	{
		if (!m_entityRegistry.Contains(entityId))
		{
			return Entity::Null();
		}

		return { m_entityRegistry.GetHandleFromID(entityId), this };
	}

	Entity EntityScene::GetEntityFromHandle(entt::entity entityHandle) const
	{
		if (!m_entityRegistry.Contains(entityHandle))
		{
			return Entity::Null();
		}

		return { entityHandle, this };
	}

	uint32_t EntityScene::GetEntityAliveCount() const
	{
		return static_cast<uint32_t>(m_registry.alive());
	}

	void EntityScene::Initialize()
	{
		m_scriptingEngine = CreateUnique<ScriptingEngine>();
		m_ecsBuilder = CreateUnique<ECSBuilder>(*m_scriptingEngine);
		m_registry.set_user_data(this);

		ComponentRegistry::Helpers::SetupComponentCallbacks(m_registry);
		ECSSystemRegistry::Get().Build(*m_ecsBuilder);
		m_ecsBuilder->Compile();
	}

	void EntityScene::ComponentOnStart()
	{
		VT_PROFILE_FUNCTION();

		for (auto&& curr : m_registry.storage())
		{
			auto& storage = curr.second;
			std::string_view tempTypeName = storage.type().name();
			StringView typeName(tempTypeName.data(), tempTypeName.size());

			const ICommonTypeDesc* typeDesc = Volt::ComponentRegistry::Get().GetTypeDescFromName(typeName);
			if (!typeDesc)
			{
				continue;
			}

			if (typeDesc->GetValueType() != ValueType::Component)
			{
				continue;
			}

			for (auto& entity : storage)
			{
				const IComponentTypeDesc* compTypeDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);
				auto entityHelper = GetEntityFromHandle(entity);
				compTypeDesc->OnStart(entityHelper);
			}
		}
	}

	void EntityScene::ComponentOnStop()
	{
		VT_PROFILE_FUNCTION();

		for (auto&& curr : m_registry.storage())
		{
			auto& storage = curr.second;
			std::string_view tempTypeName = storage.type().name();
			StringView typeName(tempTypeName.data(), tempTypeName.size());

			const ICommonTypeDesc* typeDesc = Volt::ComponentRegistry::Get().GetTypeDescFromName(typeName);
			if (!typeDesc)
			{
				continue;
			}

			if (typeDesc->GetValueType() != ValueType::Component)
			{
				continue;
			}

			for (auto& entity : storage)
			{
				const IComponentTypeDesc* compTypeDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);
				auto entityHelper = GetEntityFromHandle(entity);
				compTypeDesc->OnStop(entityHelper);
			}
		}
	}
}
