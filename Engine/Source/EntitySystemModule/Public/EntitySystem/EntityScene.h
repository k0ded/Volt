#pragma once

#include "EntitySystem/EntityTransformCache.h"
#include "EntitySystem/EntityRegistry.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/UUID.h>

#include <entt.hpp>

#include <shared_mutex>

class ECSBuilder;
class ScriptingEngine;

namespace Volt
{
	class Entity;

	class IComponentTypeDesc;
	class RenderScene;

	using TransformChangedCallbackFunc = std::function<void(Entity entity)>;
	using EntityDestroyedCallbackFunc = std::function<void(Entity entity)>;

	class VTES_API EntityScene
	{
	public:
		EntityScene();
		~EntityScene();

		void OnRuntimeStart();
		void OnRuntimeEnd();
		bool IsPlaying() const { return m_isPlaying; }

		void Update(float deltaTime);
		void FixedUpdate(float deltaTime);

		void SortScene();
		void ClearScene();

		Entity CreateEntity(const std::string& tag = "");
		Entity CreateEntityWithID(EntityID id);
		//this will create an entity WITHOUR components, but still adding an entry in the EntityRegistry
		//intended for when the caller wants to add all components at a later stage, e.g serialization
		Entity CreateEntityWithNoComponentsForID(EntityID id);

		void DestroyEntity(EntityID id,Vector<EntityID>* outDestroyedEntities = nullptr, bool isDestroyingChildFromParent = false, bool ignoreChildren = false);

		Vector<EntityID> InvalidateEntityTransform(EntityID entityId);

		UUID64 RegisterTransformChangedCallback(TransformChangedCallbackFunc&& callback);
		void UnregisterTransformChangedCallback(UUID64 id);

		UUID64 RegisterEntityDestroyedCallback(EntityDestroyedCallbackFunc&& callback);
		void UnregisterEntityDestroyedCallback(UUID64 id);

		VT_NODISCARD bool IsEntityValid(EntityID entityId) const;
		VT_NODISCARD TQS GetEntityWorldTQS(const Entity& entityHelper) const;
		VT_NODISCARD Entity GetEntityFromID(EntityID entityId) const;
		VT_NODISCARD Entity GetEntityFromHandle(entt::entity entityHandle) const;
		VT_NODISCARD uint32_t GetEntityAliveCount() const;

		VT_NODISCARD entt::entity GetEntityHandleFromID(EntityID id) const { return m_entityRegistry.GetHandleFromID(id); };

		VT_NODISCARD VT_INLINE entt::registry& GetRegistry() { return m_registry; }
		VT_NODISCARD VT_INLINE const entt::registry& GetRegistry() const { return m_registry; }
		VT_NODISCARD VT_INLINE const ScriptingEngine& GetSciptingEngine() const { return *m_scriptingEngine; }

		// #TODO_Ivar: Hack until we can figure out a proper structure
		VT_NODISCARD VT_INLINE RenderScene* GetRenderScene() const { return m_renderScene; }
		VT_INLINE void SetRenderScene(RenderScene* renderScene) { m_renderScene = renderScene; }

	private:
		//friend class Entity;

		void Initialize();

		void ComponentOnStart();
		void ComponentOnStop();

		entt::registry m_registry;
		EntityRegistry m_entityRegistry;

		Scope<ECSBuilder> m_ecsBuilder;
		Scope<ScriptingEngine> m_scriptingEngine;


		mutable EntityTransformCache m_transformCache;

		bool m_isPlaying = false;

		Map<UUID64, TransformChangedCallbackFunc> m_transformChangedCallbacks;
		Map<UUID64, EntityDestroyedCallbackFunc> m_entityDestroyedCallbacks;

		RenderScene* m_renderScene;
	};
}
