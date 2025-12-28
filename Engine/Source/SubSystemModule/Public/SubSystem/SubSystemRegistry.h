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
	bool RegisterSubSystem(SubSystemInclusionLevel inclusionLevel, SubSystemInitializationStage initializationStage)
	{
		const VoltGUID guid = T::GetStaticSubSystemGUID();

		if (m_registeredSubSystems.contains(guid))
		{
			return false;
		}

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


		return true;
	}

	VT_INLINE const Map<VoltGUID, RegisteredSubSystem>& GetRegisteredSubSystems() const { return m_registeredSubSystems; }

	static SubSystemRegistry& Get();

private:
	Map<VoltGUID, RegisteredSubSystem> m_registeredSubSystems;
};

#define VT_REGISTER_SUBSYSTEM(klass, inclusionLevel, initializationStage) \
	inline static bool SubSystemRegistry_ ## klass ## _Registered = SubSystemRegistry::Get().RegisterSubSystem<klass>(SubSystemInclusionLevel::inclusionLevel, SubSystemInitializationStage::initializationStage)

#define VT_DECLARE_SUBSYSTEM(guid) \
	VT_NODISCARD VT_INLINE static constexpr VoltGUID GetStaticSubSystemGUID() \
	{ \
		return guid; \
	}
