#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <AssetSystem/AssetHandle.h>

class AssetRegistryPanel : public EditorWindow
{
public:
	AssetRegistryPanel();
	~AssetRegistryPanel() override;

	void UpdateMainContent() override;

private:
	void OnOpen() override;
	void OnClose() override;


	bool CanVisualizeAssetType(AssetType type) const;
	void VisualizeAsset(const Volt::ReadOnlyAssetMetadata& metadata);
	void OnSearchChanged();

	void QueueUpdateMetadata();
	bool DispatchUpdateMetadata();

	void SwapOpportunity();

	static bool PassesFilter(Volt::ReadOnlyAssetMetadata& metadata, StringView search);

	String m_searchString;
	Vector<Volt::AssetHandle>* m_assetHandles;
	Vector<Volt::AssetHandle> m_intermediateAssetHandles_1;
	Vector<Volt::AssetHandle> m_intermediateAssetHandles_2;
	Map<Volt::AssetHandle, AssetReference<Volt::Asset>> m_forceLoadedAssets;

	bool m_updateQueued = true;
	std::atomic_bool m_updating = false;
	std::atomic_bool m_wantsSwap = false;

	UUID64 m_assetChangedCallbackID;
};
