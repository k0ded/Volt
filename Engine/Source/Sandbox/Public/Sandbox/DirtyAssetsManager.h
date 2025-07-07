#pragma once

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetType.h>

#include <CoreUtilities/Containers/Vector.h>

#include <set>

struct SaveDirtyAssetsFilter
{
	//todo_fabian: make a filter thingy
};

class DirtyAssetsManager
{
public:
	typedef std::function<void(Volt::AssetHandle)> DirtySaveCustomizationFn;
public:
	static DirtyAssetsManager& Get();

	void RegisterSaveCustomizationForType(AssetType type, DirtySaveCustomizationFn fn);

	void SaveAssets(SaveDirtyAssetsFilter* filter = nullptr);

	bool IsAssetDirty(Volt::AssetHandle handle);
	void MarkAssetDirty(Volt::AssetHandle handle);
	void MarkAssetNotDirty(Volt::AssetHandle handle);

	const std::set<Volt::AssetHandle>& GetDirtyAssets();

private:
	DirtyAssetsManager() = default;
	static DirtyAssetsManager s_instance;

	std::set<Volt::AssetHandle> m_dirtyAssets;	
	Map<AssetType, DirtySaveCustomizationFn> m_dirtySaveCustomizations;

};
