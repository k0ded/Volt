#pragma once

#include "Volt-Scene/WorldEngine/WorldEngine.h"
#include "Volt-Scene/Config.h"
#include "Volt-Scene/AssetTypes.h"

#include <AssetSystem/Asset_New.h>
#include <AssetSystem/AssetReference.h>

#include <EventSystem/EventListener.h>
#include <EntitySystem/EntityScene.h>

namespace Volt
{
	class Vision;
	class Entity;

	class Animation;
	class Skeleton;

	class Entity;
	class RenderScene;
	class EntityPhysicsScene;

	struct SceneSettings
	{
		bool useWorldEngine = true;
	};

	class VTS_API Scene : public Asset_New, public EventListener
	{
	public:
		struct Statistics
		{
			uint32_t entityCount = 0;
		};

		Scene();
		Scene(const std::string& name);
		~Scene() override;

		void OnRuntimeStart();
		void OnRuntimeEnd();

		void OnSimulationStart();
		void OnSimulationEnd();

		void Update(float aDeltaTime);
		void UpdateEditor(float aDeltaTime);
		void UpdateSimulation(float aDeltaTime);

		void SortScene();

		void LoadEntities();
		void UnloadEntities();
		bool IsFinishedLoadingEntities() { return m_isFinishedLoadingEntities; }

		VT_NODISCARD TQS GetEntityWorldTQS(const Entity& entity) const;

		//VT_NODISCARD VT_INLINE entt::registry& GetRegistry() { return m_entityScene.GetRegistry(); }
		VT_NODISCARD VT_INLINE EntityScene& GetEntityScene() { return m_entityScene; }
		VT_NODISCARD VT_INLINE const std::string& GetName() const { return m_name; }
		VT_NODISCARD VT_INLINE const Statistics& GetStatistics() const { return m_statistics; }
		VT_NODISCARD VT_INLINE bool IsPlaying() const { return m_isPlaying; }
		VT_NODISCARD VT_INLINE float GetDeltaTime() const { return m_currentDeltaTime; }

		VT_NODISCARD VT_INLINE SceneSettings& GetSceneSettingsMutable() { return m_sceneSettings; }
		VT_NODISCARD VT_INLINE const SceneSettings& GetSceneSettings() const { return m_sceneSettings; }
		VT_NODISCARD VT_INLINE const WorldEngine& GetWorldEngine() const { return m_worldEngine; }
		VT_NODISCARD VT_INLINE WorldEngine& GetWorldEngineMutable() { return m_worldEngine; }
		//VT_NODISCARD VT_INLINE Vision& GetVision() { return *m_visionSystem; } // #TODO_Scene

		VT_NODISCARD VT_INLINE Ref<RenderScene> GetRenderScene() const { return m_renderScene; }

		void SetRenderSize(uint32_t aWidth, uint32_t aHeight);

		Entity CreateEntity(const std::string& tag = "");
		Entity CreateEntityWithID(const EntityID& id);
		Entity CreateEntityWithIDForExistingDescription(const EntityID& id, Volt::AssetHandle existingEntityDescHandle);

		Volt::AssetHandle CreateEntityDescForEntity(const EntityID& id);

		Entity GetEntityFromID(const EntityID id) const;
		Entity GetEntityFromHandle(entt::entity entityHandle) const;
		Volt::AssetHandle GetEntityDescHandleFromEntityID(EntityID entityID) const;

		bool IsRelatedTo(Entity entity, Entity otherEntity);
		void DestroyEntity(Entity entity, bool ignoreChildren = false);
		void DestroyEntity(Entity entity, Vector<EntityID>& outDestroyedEntities, bool ignoreChildren = false);
		void DestroyEntity(Entity entity, Vector<Volt::AssetHandle>& outDestroyedEntityDescs, bool ignoreChildren = false);
		void DestroyEntity(Entity entity, Vector<EntityID>* outDestroyedEntities, Vector<Volt::AssetHandle>* outDestroyedEntityDescs, bool ignoreChildren = false);

		void InvalidateEntityTransform(const EntityID& entityId);
		bool IsEntityValid(EntityID entityId) const;

		template<typename... T>
		Vector<Entity> GetAllEntitiesWith() const;

		template<typename... T>
		Vector<Entity> GetAllEntitiesWith();

		template<typename... T, typename F>
		void ForEachWithComponents(const F& func);

		template<typename EntityType>
		Entity GetSceneEntityFromScriptingEntity(EntityType scriptingEntity);

		Vector<Entity> GetAllEntities() const;

		static AssetReference<Scene> CreateDefaultScene(const std::string& name, bool createDefaultMesh = true, bool asMemoryAsset = false);

		static AssetType GetStaticType() { return AssetTypes::Scene; }
		AssetType GetType() const override { return GetStaticType(); }
		uint32_t GetVersion() const override { return 1; }

		//copy all the entities into another scene, first removing all entities in the other scene
		void CopyEntitiesTo(AssetReference<Scene> otherScene);
		void Clear();

	private:
		friend class Entity;
		friend class SceneImporter;
		friend class SceneSerializer;

		void Initialize();
		void CreatePhysicsScene();

		void IsRecursiveChildOf(Entity mainParent, Entity currentEntity, bool& outChild);
		void ConvertToWorldSpace(Entity entity);
		void ConvertToLocalSpace(Entity entity);

		glm::mat4 GetWorldTransform(Entity entity) const;
		Vector<Entity> FlattenEntityHeirarchy(Entity entity);

		bool m_isFinishedLoadingEntities = false;

		SceneSettings m_sceneSettings;
		Statistics m_statistics;
		WorldEngine m_worldEngine;

		bool m_isPlaying = false;
		float m_timeSinceStart = 0.f;
		float m_currentDeltaTime = 0.f;

		std::string m_name = "New Scene";

		uint32_t m_viewportWidth = 1;
		uint32_t m_viewportHeight = 1;

		EntityScene m_entityScene;

		Map<Volt::EntityID, Volt::AssetHandle> m_entityIDToDescHandle;

		Ref<RenderScene> m_renderScene;
		Scope<EntityPhysicsScene> m_entityPhysicsScene;
	};

	template<typename ...T>
	inline Vector<Entity> Scene::GetAllEntitiesWith()
	{
		Vector<Entity> result{};

		auto view = m_entityScene.GetRegistry().view<T...>();
		for (const auto& ent : view)
		{
			result.emplace_back(GetEntityFromHandle(ent));
		}

		return result;
	}

	template<typename ...T>
	inline Vector<Entity> Scene::GetAllEntitiesWith() const
	{
		Vector<Entity> result{};

		auto view = m_entityScene.GetRegistry().view<T...>();
		for (const auto& ent : view)
		{
			result.emplace_back(GetEntityFromHandle(ent));
		}

		return result;
	}

	template<typename... T, typename F>
	inline void Scene::ForEachWithComponents(const F& func)
	{
		using ComponentTuple = std::tuple<T...>;
		using FirstComponentType = std::tuple_element_t<0, ComponentTuple>;

		if constexpr (std::tuple_size_v<ComponentTuple> > 1)
		{
			auto view = m_entityScene.GetRegistry().view<T...>().use<FirstComponentType>();
			view.each(func);
		}
		else
		{
			auto view = m_entityScene.GetRegistry().view<T...>();
			view.each(func);
		}
	}

	template<typename EntityType>
	inline Entity Scene::GetSceneEntityFromScriptingEntity(EntityType scriptingEntity)
	{
		return Entity{ scriptingEntity.GetHandle(), this };
	}
}
