#pragma once

#include "EntitySystem/Config.h"
#include "EntitySystem/Scripting/ECSEnvironmentDefinition.h"

#include <CoreUtilities/CompilerTraits.h>
#include <CoreUtilities/Containers/Vector.h>

#include <functional>

class ECSBuilder;

class VTES_API ECSSystemRegistry
{
public:
	bool RegisterECSModule(std::function<void(ECSBuilder& builder)> func);
	void Build(ECSBuilder& builder);
	void ClearRegistry();

	template<typename EnvType>
	bool RegisterECSEnvType()
	{
		constexpr TypeTraits::TypeIndex envTypeIndex = TypeTraits::TypeIndex::FromType<EnvType>();

		auto it = std::find_if(m_registeredEnvironmentDefinitions.begin(), m_registeredEnvironmentDefinitions.end(), [](const auto& def) { return def.typeIndex == envTypeIndex; });
		const bool hasBeenRegistered = it != m_registeredEnvironmentDefinitions.end();
		// #TODO_Ivar: Because static lib reasons this will happen for now.
		//VT_ENSURE_MSG(!hasBeenRegistered, "Type can not be registered more than once!");

		if (hasBeenRegistered)
		{
			return false;
		}

		auto& newDefinition = m_registeredEnvironmentDefinitions.emplace_back();
		newDefinition.typeIndex = envTypeIndex;
		newDefinition.typeSize = sizeof(EnvType);
		newDefinition.construct = [](void* dataPtr)
		{
			new (dataPtr) EnvType();
		};

		newDefinition.destruct = [](void* dataPtr)
		{
			EnvType* envPtr = reinterpret_cast<EnvType*>(dataPtr);
			envPtr->~EnvType();
		};

		return true;
	}

	VT_NODISCARD VT_INLINE const Vector<ECSEnvironmentDefinition>& GetEnvironmentDefinitions() const { return m_registeredEnvironmentDefinitions; }

private:
	Vector<std::function<void(ECSBuilder& builder)>> m_registeredModules;
	Vector<ECSEnvironmentDefinition> m_registeredEnvironmentDefinitions;
};

extern VTES_API ECSSystemRegistry g_ecsSystemRegistry;

VT_INLINE ECSSystemRegistry& GetECSSystemRegistry()
{
	return g_ecsSystemRegistry;
}

#define VT_REGISTER_ECS_MODULE(func) inline static bool func ## _registered = ::GetECSSystemRegistry().RegisterECSModule(func)
#define VT_REGISTER_ECS_ENV_TYPE(type) inline static bool type ## _envRegistered = ::GetECSSystemRegistry().RegisterECSEnvType<type>()
