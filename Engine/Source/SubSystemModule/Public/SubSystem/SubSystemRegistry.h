#pragma once

#include "SubSystem/Config.h"
#include "SubSystem/SubSystemInitializationStage.h"
#include "SubSystem/SubSystemDependencyList.h"

#include <CoreUtilities/CompilerTraits.h>
#include <CoreUtilities/VoltGUID.h>

#include <CoreUtilities/Containers/Map.h>

class SubSystem;

template<typename T>
concept SubSystemHasDependencies = requires
{
	{ T::GetSubSystemDependencies(std::declval<SubSystemDependencyList&>()) };
};

struct RegisteredSubSystem
{
	std::function<Ref<SubSystem>()> factoryFunction;
	std::function<void(SubSystemDependencyList&)> getDependenciesFunction;

	SubSystemInitializationStage initializationStage;
	SubSystemInclusionLevel inclusionLevel;
};

class SUBSYSTEMMODULE_API SubSystemRegistry
{
public:
	SubSystemRegistry();
	~SubSystemRegistry();

	SubSystemRegistry(const SubSystemRegistry&) = delete;
	SubSystemRegistry& operator=(const SubSystemRegistry&) = delete;

	template<typename T>
	void RegisterSubSystem(SubSystemInclusionLevel inclusionLevel, SubSystemInitializationStage initializationStage)
	{
		const VoltGUID guid = T::GetStaticSubSystemGUID();

		VT_ENSURE(!m_registeredSubSystems.contains(guid));

		RegisteredSubSystem& registeredSubSystem = m_registeredSubSystems[guid];
		registeredSubSystem.initializationStage = initializationStage;
		registeredSubSystem.inclusionLevel = inclusionLevel;
		registeredSubSystem.factoryFunction = []() 
		{
			return CreateRef<T>();
		};

		registeredSubSystem.getDependenciesFunction = [](SubSystemDependencyList& dependencyList)
		{
			if constexpr (SubSystemHasDependencies<T>)
			{
				T::GetSubSystemDependencies(dependencyList);
			}
		};
	}

	template<typename T>
	void UnregisterSubSystem()
	{
		const VoltGUID guid = T::GetStaticSubSystemGUID();

		// #Note_Ivar: The registry may already have been destroyed due to
		// DLL ordering.
		if (m_registeredSubSystems.empty())
		{
			return;
		}

		if (VT_CHECK(m_registeredSubSystems.contains(guid)))
		{
			m_registeredSubSystems.erase(guid);
		}
	}

	VT_INLINE const Map<VoltGUID, RegisteredSubSystem>& GetRegisteredSubSystems() const { return m_registeredSubSystems; }

	static SubSystemRegistry& Get();

private:
	Map<VoltGUID, RegisteredSubSystem> m_registeredSubSystems;
};

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_SUBSYSTEM(klass, inclusionLevel, initializationStage) \
	class SubSystemRegistrar_##klass \
	{ \
	public: \
		VT_INLINE SubSystemRegistrar_##klass() \
		{ \
			SubSystemRegistry::Get().RegisterSubSystem<klass>(SubSystemInclusionLevel::inclusionLevel, SubSystemInitializationStage::initializationStage); \
		} \
		VT_INLINE ~SubSystemRegistrar_##klass() \
		{ \
			SubSystemRegistry::Get().UnregisterSubSystem<klass>(); \
		} \
	} g_subSystemRegistrar_##klass

#define VT_DECLARE_SUBSYSTEM(guid) \
	VT_NODISCARD VT_INLINE static constexpr VoltGUID GetStaticSubSystemGUID() \
	{ \
		return guid; \
	}
