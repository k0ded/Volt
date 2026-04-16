#pragma once

#include "EntitySystem/Config.h"
#include "EntitySystem/Scripting/ECSEnvironmentDefinition.h"

#include <CoreUtilities/CompilerTraits.h>
#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/VoltGUID.h>

#include <functional>

class ECSBuilder;

class VTES_API ECSSystemRegistry
{
public:
	void RegisterECSModule(std::function<void(ECSBuilder& builder)> func, const VoltGUID& guid);
	void UnregisterECSModule(const VoltGUID& guid);

	void Build(ECSBuilder& builder);
	void ClearRegistry();

	template<typename EnvType>
	void RegisterECSEnvType()
	{
		constexpr TypeTraits::TypeIndex envTypeIndex = TypeTraits::TypeIndex::FromType<EnvType>();

		auto it = std::find_if(m_registeredEnvironmentDefinitions.begin(), m_registeredEnvironmentDefinitions.end(), [envTypeIndex](const auto& def) { return def.typeIndex == envTypeIndex; });
		VT_MAYBE_UNUSED const bool hasBeenRegistered = it != m_registeredEnvironmentDefinitions.end();
		VT_ENSURE_MSG(!hasBeenRegistered, "Type can not be registered more than once!");

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
	}

	template<typename EnvType>
	void UnregisterECSEnvType()
	{
		constexpr TypeTraits::TypeIndex envTypeIndex = TypeTraits::TypeIndex::FromType<EnvType>();

		if (VT_CHECK(m_registeredEnvironmentDefinitions.contains_with_predicate([envTypeIndex](const ECSEnvironmentDefinition& env) { return env.typeIndex == envTypeIndex; })))
		{
			m_registeredEnvironmentDefinitions.erase_with_predicate([envTypeIndex](const ECSEnvironmentDefinition& env) { return env.typeIndex == envTypeIndex; });
		}
	}

	VT_NODISCARD VT_INLINE const Vector<ECSEnvironmentDefinition>& GetEnvironmentDefinitions() const { return m_registeredEnvironmentDefinitions; }

	static ECSSystemRegistry& Get();

private:
	struct RegisteredModule
	{
		std::function<void(ECSBuilder& builder)> func;
		VoltGUID guid;
	};

	Map<VoltGUID, RegisteredModule> m_registeredModules;
	Vector<ECSEnvironmentDefinition> m_registeredEnvironmentDefinitions;
};

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_ECS_MODULE(func, guid) \
	static class ECSModuleRegistrar_##func \
	{ \
	public: \
		VT_INLINE ECSModuleRegistrar_##func() \
		{ \
			ECSSystemRegistry::Get().RegisterECSModule(func, guid); \
		} \
		VT_INLINE ~ECSModuleRegistrar_##func() \
		{ \
			ECSSystemRegistry::Get().UnregisterECSModule(guid); \
		} \
	} g_ecsModuleRegistrar_##func

#define VT_REGISTER_ECS_ENV_TYPE(type) \
	static class ECSEnvTypeRegistrar_##type \
	{ \
	public: \
		VT_INLINE ECSEnvTypeRegistrar_##type() \
		{ \
			ECSSystemRegistry::Get().RegisterECSEnvType<type>(); \
		} \
		VT_INLINE ~ECSEnvTypeRegistrar_##type() \
		{ \
			ECSSystemRegistry::Get().UnregisterECSEnvType<type>(); \
		} \
	} g_ecsEnvTypeRegistrar_##type
