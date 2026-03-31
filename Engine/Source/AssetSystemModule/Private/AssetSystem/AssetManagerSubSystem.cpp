#include "aspch.h"

#include "AssetSystem/AssetManagerSubSystem.h"
#include "AssetSystem/AssetManager.h"

#include <CoreModule/Project/ProjectManager.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(AssetManagerSubSystem, Minimal, Engine);

	void AssetManagerSubSystem::Initialize()
	{
		g_assetManager = CreateUnique<AssetManager>(ProjectManager::GetEngineRootDirectory(), ProjectManager::GetRootDirectory(), ProjectManager::GetAssetsDirectoryName());
	}

	void AssetManagerSubSystem::Shutdown()
	{
		g_assetManager.Reset();
	}

	void AssetManagerSubSystem::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<ProjectManager>();
	}
}
