#include "espch.h"

#include "EntitySystem/EntityRegistry.h"

namespace Volt
{
	//todo_fabian: reimplement
	//void EntityRegistry::MarkEntityAsEdited(const EntityHelper& entity)
	//{
	//	m_editedEntities.emplace(entity.GetID());
	//}

	//void EntityRegistry::ClearEditedEntities()
	//{
	//	m_editedEntities.clear();
	//	m_removedEntities.clear();
	//}

	void EntityRegistry::AddEntity(const EntityID& entityId, entt::entity entityHandle)
	{
		{
			std::shared_lock<std::shared_mutex> lock(m_entityMutex);
			if (m_entityMap.contains(entityId) || m_handleMap.contains(entityHandle))
			{
				return;
			}
		}

		std::unique_lock<std::shared_mutex> lock(m_entityMutex);
		m_entityMap.emplace(entityId, entityHandle);
		m_handleMap.emplace(entityHandle, entityId);
	}

	void EntityRegistry::RemoveEntity(const EntityID& entityId, entt::entity entityHandle)
	{
		std::unique_lock<std::shared_mutex> lock(m_entityMutex);

		if (m_entityMap.contains(entityId))
		{
			m_entityMap.erase(entityId);
		}

		if (m_handleMap.contains(entityHandle))
		{
			m_handleMap.erase(entityHandle);
		}

		//todo_fabian: reimplement
		/*if (m_editedEntities.contains(entityId))
		{
			m_editedEntities.erase(entityId);
		}

		m_removedEntities.emplace(entityId);*/
	}

	EntityID EntityRegistry::GetUUIDFromHandle(entt::entity handle) const
	{
		std::shared_lock<std::shared_mutex> lock(m_entityMutex);

		if (!m_handleMap.contains(handle))
		{
			return EntityID::Null();
		}

		return m_handleMap.at(handle);
	}

	entt::entity EntityRegistry::GetHandleFromID(EntityID uuid) const
	{
		std::shared_lock<std::shared_mutex> lock(m_entityMutex);
		if (!m_entityMap.contains(uuid))
		{
			return entt::null;
		}

		return m_entityMap.at(uuid);
	}

	bool EntityRegistry::Contains(EntityID uuid) const
	{
		std::shared_lock<std::shared_mutex> lock(m_entityMutex);
		return m_entityMap.contains(uuid);
	}

	bool EntityRegistry::Contains(entt::entity handle) const
	{
		std::shared_lock<std::shared_mutex> lock(m_entityMutex);
		return m_handleMap.contains(handle);
	}
}
