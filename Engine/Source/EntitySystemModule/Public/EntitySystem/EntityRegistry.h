#pragma once

#include "EntitySystem/EntityID.h"

#include <CoreUtilities/Containers/Map.h>

#include <entt.hpp>

#include <shared_mutex>

namespace Volt
{
	class EntityRegistry
	{
	public:
		//todo_fabian: reimplement
		//void MarkEntityAsEdited(const EntityHelper& entity);
		//void ClearEditedEntities();

		void AddEntity(const EntityID& entityId, entt::entity entityHandle);
		void RemoveEntity(const EntityID& entityId, entt::entity entityHandle);

		EntityID GetUUIDFromHandle(entt::entity handle) const;
		entt::entity GetHandleFromID(EntityID uuid) const;

		bool Contains(EntityID uuid) const;
		bool Contains(entt::entity handle) const;

		//todo_fabian: reimplement
		//inline const std::set<EntityID>& GetEditedEntities() const { return m_editedEntities; }
		//inline const std::set<EntityID>& GetRemovedEntities() const { return m_removedEntities; }

	private:
		Map<EntityID, entt::entity> m_entityMap;
		Map<entt::entity, EntityID> m_handleMap;

		//todo_fabian: reimplement
		//std::set<EntityID> m_editedEntities;
		//std::set<EntityID> m_removedEntities;

		mutable std::shared_mutex m_entityMutex;
	};
}
