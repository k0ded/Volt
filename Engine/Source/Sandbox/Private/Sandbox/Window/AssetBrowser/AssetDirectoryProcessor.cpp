#include "sbpch.h"
#include "Sandbox/Window/AssetBrowser/AssetDirectoryProcessor.h"

#include "Sandbox/Window/AssetBrowser/DirectoryItem.h"
#include "Sandbox/Window/AssetBrowser/AssetItem.h"
#include "Sandbox/Window/AssetBrowser/AssetCommon.h"
#include "Sandbox/Utility/EditorUtilities.h"

#include <FileSystemModule/Iterators/DirectoryIterator.h>

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/Profiling/Profiling.h>

AssetDirectoryProcessor::AssetDirectoryProcessor(Weak<AssetBrowser::SelectionManager> selectionManager, std::set<AssetType> assetMask, AssetBrowser::DirectoryItemAllocator& inDirItemAllocator, AssetBrowser::AssetItemAllocator& inAssetItemAllocator)
	: m_selectionManager(selectionManager), m_assetMask(assetMask), m_directoryItemAllocatorRef(inDirItemAllocator), m_assetItemAllocatorRef(inAssetItemAllocator)
{}

RawPtr<AssetBrowser::DirectoryItem> AssetDirectoryProcessor::ProcessDirectories(const Filesystem::Path& path, AssetData& meshToImportData)
{
	VT_PROFILE_FUNCTION();

	struct AssetEntryData
	{
		Filesystem::Path path = "";
		bool isDirectory = false;
		Volt::AssetHandle handle = Volt::Asset::Null();
	};

	Vector<AssetEntryData> assetEntries;

	Vector<Filesystem::Path> pathsToProcess;
	pathsToProcess.emplace_back(path);

	{
		VT_PROFILE_SCOPE("Find directories");

		while (!pathsToProcess.empty())
		{
			auto currentPath = pathsToProcess.back();
			pathsToProcess.pop_back();

			for (const auto& entry : Filesystem::DirectoryIterator(currentPath))
			{
				constexpr StringView AssetExtension = ".vtasset";
				if (entry.path.Extension() == AssetExtension)
				{
					continue;
				}

				auto& assetEntry = assetEntries.emplace_back();
				assetEntry.path = entry.path;
				assetEntry.isDirectory = entry.isDirectory;

				if (assetEntry.isDirectory)
				{
					pathsToProcess.emplace_back(assetEntry.path);
				}
			}
		}
	}

	{
		VT_PROFILE_SCOPE("Find Assets");

		Volt::AssetRegistryIteratorFilter filter;
		filter.includeMemoryAssets = false;
		filter.includeWithoutFilepath = false;
		g_assetManager->IterateAssetRegistryWithFilter(filter, [&](Volt::ReadOnlyAssetMetadata metadata) -> bool
		{
			Filesystem::Path filesystemPath = g_assetManager->GetAssetFilesystemPath(metadata->handle);
			//!filesystemPath.starts_with(path)
			if (path.Compare(filesystemPath) > 0)
			{
				return true;
			}

			auto& assetEntry = assetEntries.emplace_back();
			assetEntry.path = metadata->filepath;
			assetEntry.isDirectory = false;
			assetEntry.handle = metadata->handle;
			return true;
		});
	}


	auto relStartPath = g_assetManager->GetRelativeAssetFilepath(path);
	RawPtr<AssetBrowser::DirectoryItem> resultItem = m_directoryItemAllocatorRef.Allocate(m_selectionManager.Lock().GetRaw(), relStartPath);
	std::unordered_map<Filesystem::Path, RawPtr<AssetBrowser::DirectoryItem>> directoryItems;
	directoryItems[relStartPath] = resultItem;

	{
		VT_PROFILE_SCOPE("Create items");

		for (const auto& entry : assetEntries)
		{
			if (entry.isDirectory)
			{
				VT_PROFILE_SCOPE("Create Directory Item");
				auto relPath = g_assetManager->GetRelativeAssetFilepath(entry.path);
				RawPtr<AssetBrowser::DirectoryItem> dirData = m_directoryItemAllocatorRef.Allocate(m_selectionManager.Lock().GetRaw(), relPath);
				directoryItems[relPath] = dirData;
				const auto parentPath = g_assetManager->GetRelativeAssetFilepath(entry.path.ParentPath());
				directoryItems[parentPath]->subDirectories.emplace_back(dirData);
				dirData->parentDirectory = directoryItems[parentPath].GetRaw();
			}
			else if (entry.handle != Volt::Asset::Null())
			{
				VT_PROFILE_SCOPE("Create Asset Item");

				Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(entry.handle);

				if (!assetMetadata.IsValid())
				{
					continue;
				}

				if (assetMetadata->type != AssetTypes::None)
				{
					if (m_assetMask.empty() || m_assetMask.contains(assetMetadata->type))
					{
						RawPtr<AssetBrowser::AssetItem> assetItem = m_assetItemAllocatorRef.Allocate(m_selectionManager.Lock().GetRaw(), entry.path, meshToImportData, entry.handle);
						const auto parentPath = entry.path.ParentPath();
						if (directoryItems.contains(parentPath))
						{
							directoryItems[parentPath]->assets.emplace_back(assetItem);
						}
					}
				}
			}
			//todo: we want to show source assets aswell...
			/*else
			{
				VT_PROFILE_SCOPE("Create Non Asset Item");

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
			}*/
		}
	}

	VT_PROFILE_SCOPE("Sort Items");
	for (const auto& [dirPath, dirData] : directoryItems)
	{
		std::sort(dirData->subDirectories.begin(), dirData->subDirectories.end(), [](const RawPtr<AssetBrowser::DirectoryItem>& a, const RawPtr<AssetBrowser::DirectoryItem>& b) { return a->path.ToWString() < b->path.ToWString(); });
		std::sort(dirData->assets.begin(), dirData->assets.end(), [](const RawPtr<AssetBrowser::AssetItem>& a, const RawPtr<AssetBrowser::AssetItem>& b) { return a->path.Stem().ToWString() < b->path.Stem().ToWString(); });
	}

	return resultItem;
}
