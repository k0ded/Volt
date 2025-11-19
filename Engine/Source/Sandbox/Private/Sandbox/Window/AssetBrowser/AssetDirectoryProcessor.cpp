#include "sbpch.h"
#include "Sandbox/Window/AssetBrowser/AssetDirectoryProcessor.h"

#include "Sandbox/Window/AssetBrowser/DirectoryItem.h"
#include "Sandbox/Window/AssetBrowser/AssetItem.h"
#include "Sandbox/Window/AssetBrowser/AssetCommon.h"
#include "Sandbox/Utility/EditorUtilities.h"

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/Profiling/Profiling.h>

AssetDirectoryProcessor::AssetDirectoryProcessor(Weak<AssetBrowser::SelectionManager> selectionManager, std::set<AssetType> assetMask, AssetBrowser::DirectoryItemAllocator& inDirItemAllocator, AssetBrowser::AssetItemAllocator& inAssetItemAllocator)
	: m_selectionManager(selectionManager), m_assetMask(assetMask), m_directoryItemAllocatorRef(inDirItemAllocator), m_assetItemAllocatorRef(inAssetItemAllocator)
{}

RawPtr<AssetBrowser::DirectoryItem> AssetDirectoryProcessor::ProcessDirectories(const std::filesystem::path& path, AssetData& meshToImportData)
{
	VT_PROFILE_FUNCTION();

	struct AssetEntryData
	{
		std::filesystem::path path;
		bool isDirectory;
	};

	Vector<AssetEntryData> assetEntries;

	Vector<std::filesystem::path> pathsToProcess;
	pathsToProcess.emplace_back(path);

	{
		VT_PROFILE_SCOPE("Find directories");

		while (!pathsToProcess.empty())
		{
			auto currentPath = pathsToProcess.back();
			pathsToProcess.pop_back();

			for (const auto& entry : std::filesystem::directory_iterator(currentPath))
			{
				auto& assetEntry = assetEntries.emplace_back();
				assetEntry.path = entry.path();
				assetEntry.isDirectory = entry.is_directory();

				if (assetEntry.isDirectory)
				{
					pathsToProcess.emplace_back(assetEntry.path);
				}
			}
		}
	}

	auto relStartPath = g_assetManager->GetRelativeAssetFilepath(path);
	RawPtr<AssetBrowser::DirectoryItem> resultItem = m_directoryItemAllocatorRef.Allocate(m_selectionManager.Get(), relStartPath);
	std::unordered_map<std::filesystem::path, RawPtr<AssetBrowser::DirectoryItem>> directoryItems;
	directoryItems[relStartPath] = resultItem;

	{
		VT_PROFILE_SCOPE("Create items");

		for (const auto& entry : assetEntries)
		{
			VT_PROFILE_SCOPE("Create item");

			if (entry.isDirectory)
			{
				VT_PROFILE_SCOPE("Create Directory Item");
				auto relPath = g_assetManager->GetRelativeAssetFilepath(entry.path);
				RawPtr<AssetBrowser::DirectoryItem> dirData = m_directoryItemAllocatorRef.Allocate(m_selectionManager.Get(), relPath);
				directoryItems[relPath] = dirData;
				const auto parentPath = g_assetManager->GetRelativeAssetFilepath(entry.path.parent_path());
				directoryItems[parentPath]->subDirectories.emplace_back(dirData);
				dirData->parentDirectory = directoryItems[parentPath].GetRaw();
			}
			else
			{
				VT_PROFILE_SCOPE("Create Asset Item");

				Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(g_assetManager->GetAssetHandleFromFilepath(entry.path));

				if (!assetMetadata.IsValid())
				{
					continue;
				}

				const auto filename = entry.path.filename().string();

				if (assetMetadata->type != AssetTypes::None)
				{
					if (m_assetMask.empty() || m_assetMask.contains(assetMetadata->type))
					{
						VT_PROFILE_SCOPE("Allocate Asset Item");
						auto relPath = g_assetManager->GetRelativeAssetFilepath(entry.path);
						RawPtr<AssetBrowser::AssetItem> assetItem = m_assetItemAllocatorRef.Allocate(m_selectionManager.Get(), relPath, meshToImportData);
						const auto parentPath = g_assetManager->GetRelativeAssetFilepath(entry.path.parent_path());
						directoryItems[parentPath]->assets.emplace_back(assetItem);
					}
				}
			}
		}
	}

	for (const auto& [dirPath, dirData] : directoryItems)
	{
		std::sort(dirData->subDirectories.begin(), dirData->subDirectories.end(), [](const RawPtr<AssetBrowser::DirectoryItem>& a, const RawPtr<AssetBrowser::DirectoryItem>& b) { return a->path.string() < b->path.string(); });
		std::sort(dirData->assets.begin(), dirData->assets.end(), [](const RawPtr<AssetBrowser::AssetItem>& a, const RawPtr<AssetBrowser::AssetItem>& b) { return a->path.stem().string() < b->path.stem().string(); });
	}

	return resultItem;
}
