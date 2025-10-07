#include "espch.h"
#include "ComponentRegistry.h"

Volt::ComponentRegistry g_componentRegistry;

namespace Volt
{
	void ComponentRegistry::ClearRegistry()
	{
		m_componentHelperFunctions.clear();
		m_typeRegistry.clear();
		m_typeNameToGUIDMap.clear();
		m_guidToTypeNameMap.clear();
	}

	const ICommonTypeDesc* Volt::ComponentRegistry::GetTypeDescFromName(std::string_view name)
	{
		if (!m_typeNameToGUIDMap.contains(name))
		{
			return nullptr;
		}

		return m_typeRegistry.at(m_typeNameToGUIDMap.at(name));
	}

	const ICommonTypeDesc* ComponentRegistry::GetTypeDescFromGUID(const VoltGUID& guid)
	{
		if (!m_typeRegistry.contains(guid))
		{
			return nullptr;
		}

		return m_typeRegistry.at(guid);
	}

	std::string_view ComponentRegistry::GetTypeNameFromGUID(const VoltGUID& guid)
	{
		return m_guidToTypeNameMap.at(guid);
	}

	const VoltGUID ComponentRegistry::GetGUIDFromTypeName(std::string_view typeName)
	{
		return m_typeNameToGUIDMap.at(typeName);
	}

	void ComponentRegistry::AddComponentWithGUID(const VoltGUID& guid, entt::registry& registry, entt::entity entity)
	{
		std::unique_lock<std::shared_mutex> lock(m_componentMutexes[guid]);

		VT_ENSURE(m_componentHelperFunctions.contains(guid));
		m_componentHelperFunctions.at(guid).addComponent(registry, entity);
	}

	void ComponentRegistry::RemoveComponentWithGUID(const VoltGUID & guid, entt::registry & registry, entt::entity entity)
	{
		std::unique_lock<std::shared_mutex> lock(m_componentMutexes[guid]);
		VT_ENSURE(m_componentHelperFunctions.contains(guid));
		m_componentHelperFunctions.at(guid).removeComponent(registry, entity);
	}

	const bool ComponentRegistry::HasComponentWithGUID(const VoltGUID & guid, const entt::registry & registry, entt::entity entity)
	{
		std::shared_lock<std::shared_mutex> lock(m_componentMutexes[guid]);
		VT_ENSURE(m_componentHelperFunctions.contains(guid));
		return m_componentHelperFunctions.at(guid).hasComponent(registry, entity);
	}

	void* ComponentRegistry::GetComponentWithGUID(const VoltGUID& guid, entt::registry& registry, entt::entity entity)
	{
		std::shared_lock<std::shared_mutex> lock(m_componentMutexes[guid]);
		VT_ENSURE(m_componentHelperFunctions.contains(guid));
		return m_componentHelperFunctions.at(guid).getComponent(registry, entity);
	}

	void ComponentRegistry::SetupComponentCallbacks(entt::registry& registry)
	{
		std::unique_lock<std::shared_mutex> lock(m_helpersMutex);
		ComponentRegistry& componentRegistry = GetComponentRegistry();
		for (auto& [uuid, helpers] : componentRegistry.m_componentHelperFunctions)
		{
			helpers.setupOnCreate(registry);
			helpers.setupOnDestroy(registry);
		}
	}
}
