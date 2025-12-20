#include "sbpch.h"

#include "Sandbox/EditorAssetManager.h"

#include <CoreUtilities/FileSystem.h>

VT_DEFINE_LOG_CATEGORY(LogEditorAssetSystem);

Scope<EditorAssetManager> g_editorAssetManager;

EditorAssetManager::EditorAssetManager(Volt::AssetManager& referencedAssetManager)
	: m_referencedAssetManager(referencedAssetManager)
{
}

EditorAssetManager::~EditorAssetManager()
{
	m_assetCache.Clear();
}

void EditorAssetManager::RemoveAssetFromCache(Volt::AssetHandle assetHandle)
{
	m_assetCache.RemoveAsset(assetHandle);
}

void EditorAssetManager::RenameDirectory(const std::filesystem::path& directoryPath, const std::string& newName)
{
	VT_ENSURE(!directoryPath.empty() && !newName.empty());

	Vector<Volt::AssetHandle> assetsToRename;

	Volt::AssetRegistryIteratorFilter filter;
	filter.includeMemoryAssets = false;
	filter.includeWithoutFilepath = false;

	const std::filesystem::path absoluteDirectoryPath = m_referencedAssetManager.GetAssetFilesystemPath(directoryPath);

	m_referencedAssetManager.IterateAssetRegistryWithFilter(filter, [&](Volt::ReadOnlyAssetMetadata assetMetadata)
	{
		if (FileSystem::IsFilepathInDirectory(absoluteDirectoryPath, m_referencedAssetManager.GetAssetFilesystemPath(assetMetadata->filepath), true))
		{
			assetsToRename.emplace_back(assetMetadata->handle);
		}

		return true;
	});

	std::filesystem::path relativeDirectoryPath = m_referencedAssetManager.GetRelativeAssetFilepath(absoluteDirectoryPath);
	std::filesystem::path newRelativeDirectoryPath = relativeDirectoryPath.parent_path() / newName;

	for (const Volt::AssetHandle& assetHandle : assetsToRename)
	{
		Volt::WriteableAssetMetadata assetMetadata = m_referencedAssetManager.GetWriteableAssetMetadata(assetHandle);
		
		std::filesystem::path directoryRelativeAssetPath = std::filesystem::relative(assetMetadata->filepath, relativeDirectoryPath);
		assetMetadata->filepath = newRelativeDirectoryPath / directoryRelativeAssetPath;
	}

	FileSystem::Rename(absoluteDirectoryPath, newName);
}

void EditorAssetManager::RenameAsset(Volt::AssetHandle assetHandle, const std::string& newName)
{
	Volt::WriteableAssetMetadata assetMetadata = m_referencedAssetManager.GetWriteableAssetMetadata(assetHandle);
	if (!assetMetadata.IsValid())
	{
		VT_LOGC(Warning, LogEditorAssetSystem, "Unable to rename asset with invalid ID {}!", assetHandle);
		return;
	}

	std::filesystem::path prevAbsolutePath = m_referencedAssetManager.GetAssetFilesystemPath(assetMetadata->filepath);
	assetMetadata->filepath.replace_filename(newName + assetMetadata->filepath.extension().string());

	FileSystem::Rename(prevAbsolutePath, newName);
}

void EditorAssetManager::MoveAssetTo(Volt::AssetHandle asset, const std::filesystem::path& targetDirectory)
{
	VT_ENSURE(!targetDirectory.empty());

	Volt::WriteableAssetMetadata assetMetadata = m_referencedAssetManager.GetWriteableAssetMetadata(asset);
	if (!assetMetadata.IsValid())
	{
		VT_LOGC(Warning, LogEditorAssetSystem, "Unable to move asset with invalid ID {}!", asset);
		return;
	}

	const std::filesystem::path relativeTargetDirectory = m_referencedAssetManager.GetRelativeAssetFilepath(targetDirectory);
	const std::filesystem::path newFilepath = targetDirectory / assetMetadata->filepath.filename();

	FileSystem::Move(m_referencedAssetManager.GetAssetFilesystemPath(assetMetadata->filepath), m_referencedAssetManager.GetAssetFilesystemPath(newFilepath));

	assetMetadata->filepath = newFilepath;
}

void EditorAssetManager::MoveDirectoryTo(const std::filesystem::path& srcDirectory, const std::filesystem::path& dstDirectory)
{
	VT_ENSURE(!srcDirectory.empty() && !dstDirectory.empty());

	Vector<Volt::AssetHandle> assetsToMove;

	Volt::AssetRegistryIteratorFilter filter;
	filter.includeMemoryAssets = false;
	filter.includeWithoutFilepath = false;

	m_referencedAssetManager.IterateAssetRegistryWithFilter(filter, [&](Volt::ReadOnlyAssetMetadata assetMetadata)
	{
		if (FileSystem::IsFilepathInDirectory(srcDirectory, assetMetadata->filepath, true))
		{
			assetsToMove.emplace_back(assetMetadata->handle);
		}

		return true;
	});

	for (const Volt::AssetHandle& assetHandle : assetsToMove)
	{
		Volt::WriteableAssetMetadata assetMetadata = m_referencedAssetManager.GetWriteableAssetMetadata(assetHandle);

		std::filesystem::path srcFilepath = m_referencedAssetManager.GetAssetFilesystemPath(assetMetadata->filepath);
		std::filesystem::path srcDirRelativePath = std::filesystem::relative(assetMetadata->filepath, srcDirectory);

		assetMetadata->filepath = dstDirectory / srcDirRelativePath;

		FileSystem::Move(srcFilepath, dstDirectory / srcDirRelativePath.parent_path());
	}
}

void EditorAssetManager::DeleteAsset(Volt::AssetHandle asset)
{
	if (!m_referencedAssetManager.IsValidAssetHandle(asset))
	{
		VT_LOGC(Warning, LogEditorAssetSystem, "Asset with handle {} is not a valid asset!", asset);
		return;
	}

	std::filesystem::path assetFilepath;
	{
		Volt::ReadOnlyAssetMetadata assetMetadata = m_referencedAssetManager.GetReadOnlyAssetMetadata(asset);
		assetFilepath = m_referencedAssetManager.GetAssetFilesystemPath(assetMetadata->filepath);
	}

	m_referencedAssetManager.RemoveAsset(asset);

	if (FileSystem::Exists(assetFilepath))
	{
		FileSystem::MoveToRecycleBin(assetFilepath);
	}

	VT_LOGC(Trace, LogEditorAssetSystem, "Deleted asset {} (Handle: {})!", assetFilepath, asset);
}

void EditorAssetManager::DeleteDirectory(const std::filesystem::path& directoryPath)
{
	Vector<Volt::AssetHandle> assetsToDelete;

	Volt::AssetRegistryIteratorFilter filter;
	filter.includeMemoryAssets = false;
	filter.includeWithoutFilepath = false;

	std::filesystem::path relativeDirectoryPath = m_referencedAssetManager.GetRelativeAssetFilepath(directoryPath);

	m_referencedAssetManager.IterateAssetRegistryWithFilter(filter, [&](Volt::ReadOnlyAssetMetadata assetMetadata) 
	{
		if (FileSystem::IsFilepathInDirectory(directoryPath, assetMetadata->filepath, true))
		{
			assetsToDelete.emplace_back(assetMetadata->handle);
		}

		return true;
	});

	for (const Volt::AssetHandle& assetHandle : assetsToDelete)
	{
		DeleteAsset(assetHandle);
	}

	// The directory might have been deleted in the file system already, just make sure that it exists.
	const std::filesystem::path absoluteDirectory = m_referencedAssetManager.GetAssetFilesystemPath(relativeDirectoryPath);
	if (FileSystem::Exists(absoluteDirectory))
	{
		FileSystem::MoveToRecycleBin(absoluteDirectory);
	}

	VT_LOGC(Trace, LogEditorAssetSystem, "Deleted directory {}!", relativeDirectoryPath);
}

EditorAssetCache::EditorAssetCache()
{
	Initialize();
}

EditorAssetCache::~EditorAssetCache()
{
	VT_ENSURE_MSG(m_cache.empty(), "Cache should have been cleared before destruction!");
}

void EditorAssetCache::Clear()
{
	m_cache.clear();
}

void EditorAssetCache::AddAsset(RefPtr<Volt::Asset> asset)
{
	VT_ENSURE(asset->GetAssetHandle() != Volt::Asset::Null());

	uint64_t hashIndex;
	if (m_hashTable.Insert(asset->GetAssetHandle(), hashIndex))
	{
		m_cache[hashIndex] = asset;
	}
	else
	{
		VT_LOG(Error, "Unable to cache asset with handle '{}'", asset->GetAssetHandle());
	}
}

void EditorAssetCache::RemoveAsset(Volt::AssetHandle assetHandle)
{
	VT_ENSURE(assetHandle != Volt::Asset::Null());

	uint64_t hashIndex;
	if (m_hashTable.GetAndRemove(assetHandle, hashIndex))
	{
		m_cache[hashIndex].Reset();
	}
	else
	{
		VT_LOG(Warning, "Trying to remove asset with handle '{}' from the asset cache, but it has not been cached!", assetHandle);
	}
}

RefPtr<Volt::Asset> EditorAssetCache::GetAsset(Volt::AssetHandle assetHandle)
{
	VT_ENSURE(assetHandle != Volt::Asset::Null());

	uint64_t hashIndex;
	if (m_hashTable.Get(assetHandle, hashIndex))
	{
		return m_cache[hashIndex];
	}

	return nullptr;
}

bool EditorAssetCache::TryGetAsset(Volt::AssetHandle assetHandle, RefPtr<Volt::Asset>& outAsset)
{
	uint64_t hashIndex;
	bool found = m_hashTable.Get(assetHandle, hashIndex);
	if (found)
	{
		if (m_cache[hashIndex]->GetRefCount() > 0)
		{
			outAsset = m_cache[hashIndex];
		}
		else
		{
			return false;
		}
	}

	return found;
}

void EditorAssetCache::Initialize()
{
	m_hashTable.Reserve(Volt::AssetRegistry::GetNumMaxAssets());
	m_cache.resize(Volt::AssetRegistry::GetNumMaxAssets());
}
