#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetHandle.h"
#include "AssetSystem/AssetDependency.h"

namespace Volt
{
	struct AssetDependencyList;

	class AssetDependencyGatherContext
	{
	public:
		AssetDependencyGatherContext(AssetHandle targetAsset, AssetDependencyList& assetDependencyList);

		VTAS_API void AddDependency(AssetHandle dependencyHandle, AssetDependencyType dependencyType);

	private:
		AssetDependencyList& m_assetDependencyList;
		AssetHandle m_targetAsset;
	};
}
