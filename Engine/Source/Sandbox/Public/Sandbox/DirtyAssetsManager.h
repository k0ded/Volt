#pragma once

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetType.h>
#include <AssetSystem/AssetReference.h>

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/VectorVariants.h>

#include <EventSystem/EventListener.h>

#include <set>

namespace Volt
{
	class AssetCreatedEvent;
	class AssetSavedEvent;
	class Asset;
	enum class AssetChangedState : uint8_t;
}

struct SaveDirtyAssetsFilter
{
	//return true for an asset if it should be included in the save
	std::function<bool(const Volt::AssetHandle&/*asset*/)> includeAssetDelegate;
};

struct DirtySaveCustomization
{
	// initial check if the asset is allowed to be saved,
	// the asset will still show up in the explicit save popup but will be disabled
	std::function<bool(const Volt::AssetHandle&/*asset*/, String& /*outCantReason*/)> CanSaveAsset;

	// check if we can save asset after the create assets modal has been run
	// only called if CanUserAssignPath returned false
	std::function<bool(const Volt::AssetHandle&/*asset*/, Filesystem::Path& /*outAssetNewPath*/, String& /*outCantReason*/)> CanSaveAssetPostCreateStep;

	// check if the user is allowed to set the path of the asset manually, 
	// returning false will hide the asset in the create assets modal
	std::function<bool(const Volt::AssetHandle&/*asset*/)> CanUserAssignPath;


	// check if asset should be removed instead of saved when save is performed
	// returning true will delete the asset when save otherwise happens
	std::function<bool(const Volt::AssetHandle&/*asset*/)> ShouldDeleteInstead;
};

class DirtyAssetsManager : public SubSystem
{
public:
	static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
	VT_DECLARE_SUBSYSTEM("{DEFEC05B-66E0-45D9-8D1A-694DD2166407}"_guid)

public:
	typedef std::function<void(Volt::AssetHandle)> DirtySaveCustomizationFn;
public:
	static DirtyAssetsManager& Get();

	DirtyAssetsManager();
	~DirtyAssetsManager();

	void Initialize() override;
	void Shutdown() override;

	void RegisterSaveCustomizationForType(AssetType type, DirtySaveCustomization customization);

	//returns false if user cancels
	//allowDiscardSave is only relevant if showSaveDialog is true
	bool SaveAssets(bool showSaveDialog = false, bool allowDiscardSave = false, SaveDirtyAssetsFilter filter = SaveDirtyAssetsFilter());

	bool IsAssetDirty(Volt::AssetHandle handle);
	void MarkAssetDirty(Volt::AssetHandle handle);
	void MarkAssetNotDirty(Volt::AssetHandle handle);

	const Map<Volt::AssetHandle, AssetReference<Volt::Asset>>& GetDirtyAssets();

private:
	static DirtyAssetsManager* s_instance;

	void OnAssetChanged(Volt::AssetHandle assetHandle, Volt::AssetChangedState state);

	void SaveAssetsImpl(const GlobalMemoryStackVector<Volt::AssetHandle>& assetsToSave);
	void CreateAssetsImpl(const Vector<std::pair<Volt::AssetHandle, Filesystem::Path>>& assetsToCreate);

	Map<Volt::AssetHandle, AssetReference<Volt::Asset>> m_dirtyAssets;
	Map<AssetType, DirtySaveCustomization> m_dirtySaveCustomizations;

	UUID64 m_assetsModalID;
	UUID64 m_assetChangedCallbackID;
};
