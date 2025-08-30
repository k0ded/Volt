#pragma once

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetType.h>

#include <CoreUtilities/Containers/Vector.h>

#include <set>

struct SaveDirtyAssetsFilter
{
	//todo_fabian: make a filter thingy
	Vector<Volt::AssetHandle> SkipAssets;
};

struct RequiredExternalActionData
{
	Vector<Volt::AssetHandle> ReadOnlyAssets;
	Vector<Volt::AssetHandle> AssetsNeedingCustomAction;
};

struct DirtySaveCustomization
{
	//return true to add this asset to the required action data list
	std::function<bool(Volt::AssetHandle)> RequiresExternalAction;
};

class DirtyAssetsManager
{
public:
	typedef std::function<void(Volt::AssetHandle)> DirtySaveCustomizationFn;
public:
	static DirtyAssetsManager& Get();

	void Initialize();

	void RegisterSaveCustomizationForType(AssetType type, DirtySaveCustomization customization);

	void SaveAssets(bool showSaveDialog = true, SaveDirtyAssetsFilter filter = SaveDirtyAssetsFilter());

	bool IsAssetDirty(Volt::AssetHandle handle);
	void MarkAssetDirty(Volt::AssetHandle handle);
	void MarkAssetNotDirty(Volt::AssetHandle handle);

	const std::set<Volt::AssetHandle>& GetDirtyAssets();

private:
	DirtyAssetsManager() = default;
	static DirtyAssetsManager s_instance;

	void SaveAssetsImpl(SaveDirtyAssetsFilter filter);

	std::set<Volt::AssetHandle> m_dirtyAssets;	
	Map<AssetType, DirtySaveCustomization> m_dirtySaveCustomizations;

	UUID64 m_assetsModalID;
};
