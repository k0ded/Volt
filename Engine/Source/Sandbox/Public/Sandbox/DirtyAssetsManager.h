#pragma once

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetType.h>

#include <SubSystem/SubSystem.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/VectorVariants.h>

#include <EventSystem/EventListener.h>

#include <set>

namespace Volt
{
	class AssetCreatedEvent;
	class AssetSavedEvent;
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
	std::function<bool(const Volt::AssetHandle&/*asset*/, std::string& /*outCantReason*/)> CanSaveAsset;

	// check if we can save asset after the create assets modal has been run
	// only called if CanUserAssignPath returned false
	std::function<bool(const Volt::AssetHandle&/*asset*/, std::filesystem::path& /*outAssetNewPath*/, std::string& /*outCantReason*/)> CanSaveAssetPostCreateStep;

	// check if the user is allowed to set the path of the asset manually, 
	// returning false will hide the asset in the create assets modal
	std::function<bool(const Volt::AssetHandle&/*asset*/)> CanUserAssignPath;
};

class DirtyAssetsManager : public SubSystem, public Volt::EventListener
{
public:
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
	bool SaveAssets(bool showSaveDialog = true, SaveDirtyAssetsFilter filter = SaveDirtyAssetsFilter());

	bool IsAssetDirty(Volt::AssetHandle handle);
	void MarkAssetDirty(Volt::AssetHandle handle);
	void MarkAssetNotDirty(Volt::AssetHandle handle);

	const std::set<Volt::AssetHandle>& GetDirtyAssets();

private:
	static DirtyAssetsManager* s_instance;

	void RegisterEventListeners();

	bool OnAssetCreated(Volt::AssetCreatedEvent& e);
	bool OnAssetSaved(Volt::AssetSavedEvent& e);

	void SaveAssetsImpl(const FrameStackVector<Volt::AssetHandle>& assetsToSave);
	void CreateAssetsImpl(const Vector<std::pair<Volt::AssetHandle, std::filesystem::path>>& assetsToCreate);

	std::set<Volt::AssetHandle> m_dirtyAssets;
	Map<AssetType, DirtySaveCustomization> m_dirtySaveCustomizations;

	UUID64 m_assetsModalID;
};
