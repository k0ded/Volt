#include "sbpch.h"
#include "Sandbox/Window/AssetBrowser/AssetDirectoryProcessor.h"

#include "Sandbox/Window/AssetBrowser/DirectoryItem.h"
#include "Sandbox/Window/AssetBrowser/AssetItem.h"
#include "Sandbox/Window/AssetBrowser/AssetCommon.h"
#include "Sandbox/Utility/EditorUtilities.h"

#include <AssetSystem/AssetManager_New.h>

#include <CoreUtilities/Profiling/Profiling.h>

AssetDirectoryProcessor::AssetDirectoryProcessor(Weak<AssetBrowser::SelectionManager> selectionManager, std::set<AssetType> assetMask)
	: m_selectionManager(selectionManager), m_assetMask(assetMask)
{}

Ref<AssetBrowser::DirectoryItem> AssetDirectoryProcessor::ProcessDirectories(const std::filesystem::path& path, AssetData& meshToImportData)
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
	Ref<AssetBrowser::DirectoryItem> resultItem = CreateRef<AssetBrowser::DirectoryItem>(m_selectionManager.Get(), relStartPath);
	std::unordered_map<std::filesystem::path, Ref<AssetBrowser::DirectoryItem>> directoryItems;
	directoryItems[relStartPath] = resultItem;

	{
		VT_PROFILE_SCOPE("Create items");

		for (const auto& entry : assetEntries)
		{
			VT_PROFILE_SCOPE("Create item");

			if (entry.isDirectory)
			{
				auto relPath = g_assetManager->GetRelativeAssetFilepath(path);
				Ref<AssetBrowser::DirectoryItem> dirData = CreateRef<AssetBrowser::DirectoryItem>(m_selectionManager.Get(), relPath);
				directoryItems[relPath] = dirData;
				const auto parentPath = g_assetManager->GetRelativeAssetFilepath(entry.path.parent_path());
				directoryItems[parentPath]->subDirectories.emplace_back(dirData);
				dirData->parentDirectory = directoryItems[parentPath].get();
			}
			else
			{
				Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(g_assetManager->GetAssetHandleFromFilepath(path));

				if (!assetMetadata.IsValid())
				{
					continue;
				}

				const auto filename = entry.path.filename().string();

				if (assetMetadata->type != AssetTypes::None)
				{
					if (m_assetMask.empty() || m_assetMask.contains(assetMetadata->type))
					{
						auto relPath = g_assetManager->GetRelativeAssetFilepath(path);
						Ref<AssetBrowser::AssetItem> assetItem = CreateRef<AssetBrowser::AssetItem>(m_selectionManager.Get(), relPath, meshToImportData);
						const auto parentPath = g_assetManager->GetRelativeAssetFilepath(entry.path.parent_path());
						directoryItems[parentPath]->assets.emplace_back(assetItem);
					}
				}
			}
		}
	}

	for (const auto& [dirPath, dirData] : directoryItems)
	{
		std::sort(dirData->subDirectories.begin(), dirData->subDirectories.end(), [](const Ref<AssetBrowser::DirectoryItem>& a, const Ref<AssetBrowser::DirectoryItem>& b) { return a->path.string() < b->path.string(); });
		std::sort(dirData->assets.begin(), dirData->assets.end(), [](const Ref<AssetBrowser::AssetItem>& a, const Ref<AssetBrowser::AssetItem>& b) { return a->path.stem().string() < b->path.stem().string(); });
	}

	return resultItem;
}
