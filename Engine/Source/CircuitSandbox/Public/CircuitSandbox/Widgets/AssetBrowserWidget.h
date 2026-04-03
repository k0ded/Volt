#pragma once

#include <Circuit/Widgets/CompoundWidget.h>
#include <Circuit/Widgets/ListViewWidget.h>


#include <CoreUtilities/String/VoltString.h>
#include <CoreUtilities/Filesystem/Path.h>

#include <CoreUtilities/Containers/Vector.h>

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetType.h>

enum class AssetBrowserItemType
{
	Invalid, // Something went wrong
	Asset, // Asset existing in the AssetManager
	Directory, // Directory holding asset browser items
	SourceAsset, // Files that are not a part of the AssetManager
};


typedef UUID32 AssetBrowserItemID;
struct AssetBrowserAssetItem
{
	AssetBrowserItemID ID;

	Filesystem::Path Path = "Error";
	Volt::AssetHandle Handle = 0;
	AssetType Type = AssetTypes::None;
};

struct AssetBrowserItemProxy
{
	AssetBrowserItemType Type;
	AssetBrowserItemID ParentDirectoryID;
	AssetBrowserItemID ID;
};

struct AssetBrowserDirectory
{
	AssetBrowserItemID ID;
	Filesystem::Path Path = "Error";

	Vector<AssetBrowserItemProxy> Children;

	Map<AssetBrowserItemID, AssetBrowserAssetItem> Assets;
};

class AssetBrowserWidget : public Circuit::CompoundWidget
{
public:
	AssetBrowserWidget();
	virtual ~AssetBrowserWidget();

	CIRCUIT_BEGIN_ARGS(AssetBrowserWidget)
	{};

	CIRCUIT_END_ARGS();

	void Build(const Arguments& args);

	void RediscoverFiles();

	virtual glm::vec2 GetDesiredSize() override;

private:
	Ref<Circuit::IListViewRow<AssetBrowserItemProxy>> GenerateRow(AssetBrowserItemProxy& item);
	void OnRowDoubleClicked(AssetBrowserItemProxy& item);


	void OnClickedReload(Volt::InputCode button);
	void OnClickedUp(Volt::InputCode button);

	AssetBrowserItemID m_currentDirectoryID;
	Vector<AssetBrowserItemProxy>* m_currentDirectoryItems = nullptr;
	Map<AssetBrowserItemID, AssetBrowserDirectory> m_directories;
	Map<Filesystem::Path, AssetBrowserItemID> m_directoryPathToID;

	Ref<Circuit::ListViewWidget<AssetBrowserItemProxy>> m_assetsListWidget;

	std::atomic_bool m_reloading;
	std::atomic_int m_reloadProgress;
	std::atomic_int m_reloadTotalWork;
};
