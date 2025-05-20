#include "espch.h"

#include "EntitySystem/Scripting/ScriptingEngine.h"
#include "EntitySystem/Scripting/ECSEventDispatcher.h"
#include "EntitySystem/Scripting/ECSSystemRegistry.h"

ScriptingEngine::ScriptingEngine()
{
	m_ecsEventDispatcher = CreateScope<ECSEventDispatcher>();
}

ScriptingEngine::~ScriptingEngine()
{
	m_ecsEventDispatcher = nullptr;
}

void ScriptingEngine::OnRuntimeStart()
{
	m_ecsEnvironmentStorage.InitializeWith(GetECSSystemRegistry().GetEnvironmentDefinitions());
	m_ecsEventDispatcher->OnRuntimeStart();
}

void ScriptingEngine::OnRuntimeEnd()
{
	m_ecsEventDispatcher->OnRuntimeEnd();
	m_ecsEnvironmentStorage.Clear();
}

const void* ScriptingEngine::GetECSEnvironmentOfTypeInternal(TypeTraits::TypeIndex typeIndex) const
{
	if (m_ecsEnvironmentStorage.ContainsType(typeIndex))
	{
		return m_ecsEnvironmentStorage.GetDataPointerOfType(typeIndex);
	}

	return nullptr;
}

void* ScriptingEngine::GetECSEnvironmentOfTypeInternal(TypeTraits::TypeIndex typeIndex)
{
	if (m_ecsEnvironmentStorage.ContainsType(typeIndex))
	{
		return m_ecsEnvironmentStorage.GetDataPointerOfType(typeIndex);
	}

	return nullptr;
}
