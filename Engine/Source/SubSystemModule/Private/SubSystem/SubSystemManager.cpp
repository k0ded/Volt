#include "sspch.h"

#include "SubSystem/SubSystemManager.h"
#include "SubSystem/SubSystemDependencyList.h"
#include "SubSystem/SubSystemRegistry.h"
#include "SubSystem/SubSystem.h"

#include <ranges>

SubSystemManager::SubSystemManager(SubSystemInclusionLevel inclusionLevel)
	: m_inclusionLevel(inclusionLevel)
{
	VT_ASSERT(!s_instance);
	s_instance = this;

	BuildDependencyTree();
}

SubSystemManager::~SubSystemManager()
{
	s_instance = nullptr;
}

void SubSystemManager::InitializeSubSystems(SubSystemInitializationStage initializationStage)
{
	if (!m_sortedSubSystems.contains(initializationStage))
	{
		return;
	}

	const auto& registeredSubSystems = SubSystemRegistry::Get().GetRegisteredSubSystems();
	const Vector<VoltGUID>& sortedSubSystems = m_sortedSubSystems.at(initializationStage);

	for (const VoltGUID& subSystemGUID : sortedSubSystems)
	{
		Ref<SubSystem> subSystem = registeredSubSystems.at(subSystemGUID).factoryFunction();
		m_subSystemsMap[subSystemGUID] = subSystem;
		
		VT_LOGC(Trace, LogSubSystem, "Initialized SubSystem {}!", subSystemGUID);
		subSystem->Initialize();
	}

	for (const VoltGUID& subSystemGUID : sortedSubSystems)
	{
		m_subSystemsMap.at(subSystemGUID)->OnPostStageInitializaton();
	}
}

void SubSystemManager::ShutdownSubSystems(SubSystemInitializationStage initializationStage)
{
	// Ensure correct destructor order (event listeners may depend on this)
	if (!m_sortedSubSystems.contains(initializationStage))
	{
		return;
	}

	const Vector<VoltGUID>& sortedSubSystems = m_sortedSubSystems.at(initializationStage);

	for (const VoltGUID& subSystemGUID : std::ranges::reverse_view(sortedSubSystems))
	{
		m_subSystemsMap[subSystemGUID]->Shutdown();
		m_subSystemsMap.erase(subSystemGUID);
	}
}

void SubSystemManager::OnPostInitialization()
{
	for (const auto& [guid, subSystem] : m_subSystemsMap)
	{
		subSystem->OnPostInitialization();
	}
}

void SubSystemManager::OnPreShutdown()
{
	for (const auto& [guid, subSystem] : m_subSystemsMap)
	{
		subSystem->OnPreShutdown();
	}
}

void SubSystemManager::BuildDependencyTree()
{
	struct SubSystemReference
	{
		VoltGUID guid;
		uint32_t referenceCount;

		Vector<SubSystemReference*> dependants;
	};

	struct SubSystemInitializationStageData
	{
		Map<VoltGUID, size_t> subSystemGUIDToIndex;
		Vector<SubSystemReference> subSystemReferences;
	};

	Map<SubSystemInitializationStage, SubSystemInitializationStageData> subSystemInitializationStageData;

	const auto& registeredSubSystems = SubSystemRegistry::Get().GetRegisteredSubSystems();

	// Initialize map and list.
	for (const auto& [guid, registeredSubSystem] : registeredSubSystems)
	{
		SubSystemInitializationStageData& stageData = subSystemInitializationStageData[registeredSubSystem.initializationStage];

		SubSystemReference& reference = stageData.subSystemReferences.emplace_back();
		reference.guid = guid;
		reference.referenceCount = 0;

		stageData.subSystemGUIDToIndex[guid] = stageData.subSystemReferences.size() - 1;
	}

	// Resolve dependencies
	for (const auto& [guid, registeredSubSystem] : registeredSubSystems)
	{
		SubSystemInitializationStageData& stageData = subSystemInitializationStageData[registeredSubSystem.initializationStage];
		SubSystemReference& reference = stageData.subSystemReferences.at(stageData.subSystemGUIDToIndex[guid]);

		// Gather possible dependencies
		SubSystemDependencyList dependencyList;
		registeredSubSystem.getDependenciesFunction(dependencyList);
	
		// Resolve dependency pointers.
		for (const VoltGUID& dependencyGUID : dependencyList.GetDependencyList())
		{
			if (stageData.subSystemGUIDToIndex.contains(dependencyGUID))
			{
				SubSystemReference* depReference = &stageData.subSystemReferences.at(stageData.subSystemGUIDToIndex.at(dependencyGUID));

				depReference->dependants.emplace_back(&reference);
				reference.referenceCount++;
			}
			// Dependency sub system is not in the same initialization stage.
			// It's invalid to have a dependency in a stage later than itself.
			else
			{
				for (const auto& [depStage, depStageData] : subSystemInitializationStageData)
				{
					if (static_cast<std::underlying_type_t<SubSystemInitializationStage>>(depStage) >
						static_cast<std::underlying_type_t<SubSystemInitializationStage>>(registeredSubSystem.initializationStage))
					{
						VT_ENSURE_MSG(!depStageData.subSystemGUIDToIndex.contains(dependencyGUID), "Dependency SubSystem is in a later initialization stage than it's dependant! This is not allowed!");
					}
				}
			}
		}
	}

	// Topological sort
	for (auto& [stage, stageData] : subSystemInitializationStageData)
	{
		Vector<SubSystemReference*> unreferencedSubSystems;
		for (size_t i = 0; i < stageData.subSystemReferences.size(); ++i)
		{
			if (stageData.subSystemReferences[i].referenceCount == 0)
			{
				unreferencedSubSystems.emplace_back(&stageData.subSystemReferences[i]);
			}
		}

		Vector<VoltGUID>& sortedSubSystems = m_sortedSubSystems[stage];
		sortedSubSystems.reserve(stageData.subSystemReferences.size());

		while (!unreferencedSubSystems.empty())
		{
			SubSystemReference* subSystemReference = unreferencedSubSystems.back();
			unreferencedSubSystems.pop_back();

			sortedSubSystems.emplace_back(subSystemReference->guid);

			for (SubSystemReference* dependantSubSystem : subSystemReference->dependants)
			{
				dependantSubSystem->referenceCount--;
				if (dependantSubSystem->referenceCount == 0)
				{
					unreferencedSubSystems.emplace_back(dependantSubSystem);
				}
			}
		}
	}
}
