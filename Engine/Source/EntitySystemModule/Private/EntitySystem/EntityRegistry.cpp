#include "espch.h"

#include "EntitySystem/EntityRegistry.h"
#include "EntitySystem/EntityHelper.h"

namespace Volt
{
	void EntityRegistry::MarkEntityAsEdited(const EntityHelper& entity)
	{
		m_editedEntities.emplace(entity.GetID());
	}

	void EntityRegistry::ClearEditedEntities()
	{
		m_editedEntities.clear();
		m_removedEntities.clear();
	}

	void EntityRegistry::AddEntity(const EntityHelper& entity)
	{
		WriteLock lock{ m_mutex };

		if (m_entityMap.contains(entity.GetID()) || m_handleMap.contains(entity.GetHandle()))
		{
			return;
		}

		m_entityMap.emplace(entity.GetID(), entity.GetHandle());
		m_handleMap.emplace(entity.GetHandle(), entity.GetID());
	}

	void EntityRegistry::RemoveEntity(const EntityID& entityId, entt::entity entityHandle)
	{
		WriteLock lock{ m_mutex };

		if (m_entityMap.contains(entityId))
		{
			m_entityMap.erase(entityId);
		}

		if (m_handleMap.contains(entityHandle))
		{
			m_handleMap.erase(entityHandle);
		}

		if (m_editedEntities.contains(entityId))
		{
			m_editedEntities.erase(entityId);
		}

		m_removedEntities.emplace(entityId);
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
