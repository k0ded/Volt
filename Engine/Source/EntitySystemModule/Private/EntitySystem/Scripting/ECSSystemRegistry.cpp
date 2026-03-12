#include "espch.h"
#include "EntitySystem/Scripting/ECSSystemRegistry.h"

void ECSSystemRegistry::RegisterECSModule(std::function<void(ECSBuilder& builder)> func, const VoltGUID& guid)
{
	VT_ENSURE(!m_registeredModules.contains(guid));
	RegisteredModule& registeredModule = m_registeredModules[guid];
	registeredModule.func = std::move(func);
	registeredModule.guid = guid;
}

void ECSSystemRegistry::UnregisterECSModule(const VoltGUID& guid)
{
	// #Note_Ivar: The registry may already have been destroyed due to
	// DLL ordering.
	if (m_registeredModules.empty())
	{
		return;
	}

	if (VT_CHECK(m_registeredModules.contains(guid)))
	{
		m_registeredModules.erase(guid);
	}
}

void ECSSystemRegistry::Build(ECSBuilder& builder)
{
	for (const auto& [guid, module] : m_registeredModules)
	{
		module.func(builder);
	}
}

void ECSSystemRegistry::ClearRegistry()
{
	m_registeredModules.clear();
}

ECSSystemRegistry& ECSSystemRegistry::Get()
{
	static ECSSystemRegistry registry;
	return registry;
}
