#include "aspch.h"

#include "AssetSystem/AssetDependencyGatherContext.h"
#include "AssetSystem/AssetMetadata.h"

namespace Volt
{
	AssetDependencyGatherContext::AssetDependencyGatherContext(AssetHandle targetAsset, AssetDependencyList& assetDependencyList)
		: m_assetDependencyList(assetDependencyList), 
		m_targetAsset(targetAsset)
	{
	}

	void AssetDependencyGatherContext::AddDependency(AssetHandle dependencyHandle, AssetDependencyType dependencyType)
	{
		if (m_assetDependencyList.dependencies.contains_with_predicate([&](const AssetDependency& dep) { return dep.assetHandle == dependencyHandle; }))
		{
			return;
		}

		m_assetDependencyList.dependencies.emplace_back(dependencyHandle, dependencyType);
	}
}
