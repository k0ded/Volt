#include "espch.h"

#include "EntitySystem/EntityRegistry.h"

namespace Volt
{
	void EntityRegistry::AddEntity(const EntityID& entityId, entt::entity entityHandle)
	{
		{
			ReadLock lock(m_mutex);
			if (m_entityMap.contains(entityId) || m_handleMap.contains(entityHandle))
			{
				return;
			}
		}

		WriteLock lock{ m_mutex };
		m_entityMap.emplace(entityId, entityHandle);
		m_handleMap.emplace(entityHandle, entityId);
	}

	void EntityRegistry::RemoveEntity(const EntityID& entityId, entt::entity entityHandle)
	{
		WriteLock lock{ m_mutex };

		if (m_handleMap.contains(entityHandle))
		{
			m_handleMap.erase(entityHandle);
		}
	}

	EntityID EntityRegistry::GetUUIDFromHandle(entt::entity handle) const
	{
		ReadLock lock{ m_mutex };

		if (!m_handleMap.contains(handle))
		{
			return EntityID::Null();
		}

		return m_handleMap.at(handle);
	}

	entt::entity EntityRegistry::GetHandleFromID(EntityID uuid) const
	{
		ReadLock lock{ m_mutex };
		if (!m_entityMap.contains(uuid))
		{
			return entt::null;
		}

		return m_entityMap.at(uuid);
	}

	bool EntityRegistry::Contains(EntityID uuid) const
	{
		ReadLock lock{ m_mutex };
		return m_entityMap.contains(uuid);
	}

	bool EntityRegistry::Contains(entt::entity handle) const
	{
		ReadLock lock{ m_mutex };
		return m_handleMap.contains(handle);
	}
}
