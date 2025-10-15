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
		void AddEntity(const EntityID& entityId, entt::entity entityHandle);
		void RemoveEntity(const EntityID& entityId, entt::entity entityHandle);

		EntityID GetUUIDFromHandle(entt::entity handle) const;
		entt::entity GetHandleFromID(EntityID uuid) const;

		bool Contains(EntityID uuid) const;
		bool Contains(entt::entity handle) const;

	private:
		Map<EntityID, entt::entity> m_entityMap;
		Map<entt::entity, EntityID> m_handleMap;

		mutable std::shared_mutex m_entityMutex;
	};
}
