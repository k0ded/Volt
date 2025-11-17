#pragma once

#include "SubSystem/Config.h"
#include "SubSystem/SubSystemInitializationStage.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/VoltGUID.h>

class SubSystem;
class SUBSYSTEMMODULE_API SubSystemManager
{
public:
	SubSystemManager(SubSystemInclusionLevel inclusionLevel = SubSystemInclusionLevel::Default);
	~SubSystemManager();

	SubSystemManager(const SubSystemManager&) = delete;
	SubSystemManager& operator=(const SubSystemManager&) = delete;

	void InitializeSubSystems(SubSystemInitializationStage initializationStage);
	void ShutdownSubSystems(SubSystemInitializationStage initializationStage);

	template<typename T>
	static T* GetSubSystem()
	{
		constexpr VoltGUID guid = T::GetStaticSubSystemGUID();
		if (!s_instance->m_subSystemsMap.contains(guid))
		{
			return nullptr;
		}

		return reinterpret_cast<T*>(s_instance->m_subSystemsMap.at(guid).get());
	}

private:
	inline static SubSystemManager* s_instance = nullptr;

	struct SubSystemAccelerationStructure
	{
		SubSystemInitializationStage stage;
		size_t index;
	};

	Map <VoltGUID, Ref<SubSystem>> m_subSystemsMap;
	Map<SubSystemInitializationStage, Vector<Ref<SubSystem>>> m_subSystems;
	SubSystemInclusionLevel m_inclusionLevel;
};
