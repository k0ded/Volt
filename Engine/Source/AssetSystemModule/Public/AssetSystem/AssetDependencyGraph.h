#pragma once

#include "AssetSystem/AssetMetadata.h"

#include <CoreUtilities/Containers/Graph.h>

#include <shared_mutex>

namespace Volt
{
	struct AssetDependencyInfo
	{
		AssetHandle handle;
	};

	struct EdgeInfo
	{
	};

	class AssetManager;

	class AssetDependencyGraph
	{
	public:
		using WriteLock = std::unique_lock<std::shared_mutex>;
		using ReadLock = std::shared_lock<std::shared_mutex>;

		AssetDependencyGraph(AssetManager& referencedAssetManager);
		~AssetDependencyGraph();

		VTAS_API UUID64 AddAssetToGraph(AssetHandle handle);
		void RemoveAssetFromGraph(AssetHandle handle);

		void AddDependencyToAsset(AssetHandle handle, AssetHandle dependency);
		void RemoveDependencyToAsset(AssetHandle handle, AssetHandle dependency);

		void OnAssetChanged(AssetHandle handle, AssetChangedState state);

		const Vector<AssetHandle> GetAssetDependencyChain(AssetHandle handle) const;
		const Vector<AssetHandle> GetAssetsDependentOn(AssetHandle handle) const;

		inline const bool DoAssetExistInGraph(AssetHandle handle) const { return m_assetNodeIds.contains(handle); }

	private:
		mutable std::shared_mutex m_mutex;
		
		std::unordered_map<AssetHandle, UUID64> m_assetNodeIds;
		Graph<AssetDependencyInfo, EdgeInfo> m_graph;
	
		AssetManager& m_referencedAssetManager;
	};
}
