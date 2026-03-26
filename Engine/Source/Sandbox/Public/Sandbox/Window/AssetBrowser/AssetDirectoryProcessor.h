#pragma once

#include "Sandbox/Window/AssetBrowser/AssetBrowserConstants.h"

#include <AssetSystem/AssetType.h>
#include <CoreUtilities/Pointers/Weak.h>

namespace AssetBrowser
{
	class SelectionManager;
	class DirectoryItem;
}

struct AssetData;

class AssetDirectoryProcessor
{
public:
	AssetDirectoryProcessor(Weak<AssetBrowser::SelectionManager> selectionManager, std::set<AssetType> assetMask, AssetBrowser::DirectoryItemAllocator& inDirItemAllocator, AssetBrowser::AssetItemAllocator& inAssetItemAllocator);

	RawPtr<AssetBrowser::DirectoryItem> ProcessDirectories(const Filesystem::Path& path, AssetData& meshToImportData);

private:
	AssetBrowser::AssetItemAllocator& m_assetItemAllocatorRef;
	AssetBrowser::DirectoryItemAllocator& m_directoryItemAllocatorRef;

	Weak<AssetBrowser::SelectionManager> m_selectionManager;
	std::set<AssetType> m_assetMask;
};
