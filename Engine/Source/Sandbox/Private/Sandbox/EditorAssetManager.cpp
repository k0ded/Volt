#include "sbpch.h"

#include "Sandbox/EditorAssetManager.h"

#include <FileSystemModule/Filesystem.h>

VT_DEFINE_LOG_CATEGORY(LogEditorAssetSystem);

Unique<EditorAssetManager> g_editorAssetManager;

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

void EditorAssetManager::RenameDirectory(const Filesystem::Path& directoryPath, const String& newName)
{
	VT_ENSURE(!directoryPath.IsEmpty() && !newName.empty());

	Vector<Volt::AssetHandle> assetsToRename;

	Volt::AssetRegistryIteratorFilter filter;
	filter.includeMemoryAssets = false;
	filter.includeWithoutFilepath = false;

	const Filesystem::Path absoluteDirectoryPath = m_referencedAssetManager.GetAssetFilesystemPath(directoryPath);

	m_referencedAssetManager.IterateAssetRegistryWithFilter(filter, [&](Volt::ReadOnlyAssetMetadata assetMetadata)
	{
		if (Filesystem::IsFilepathInDirectory(absoluteDirectoryPath, m_referencedAssetManager.GetAssetFilesystemPath(assetMetadata->filepath), true))
		{
			assetsToRename.emplace_back(assetMetadata->handle);
		}

		return true;
	});

	Filesystem::Path relativeDirectoryPath = m_referencedAssetManager.GetRelativeAssetFilepath(absoluteDirectoryPath);
	Filesystem::Path newRelativeDirectoryPath = relativeDirectoryPath.ParentPath() / newName;

	for (const Volt::AssetHandle& assetHandle : assetsToRename)
	{
		Volt::WriteableAssetMetadata assetMetadata = m_referencedAssetManager.GetWriteableAssetMetadata(assetHandle);
		
		Filesystem::Path directoryRelativeAssetPath = Filesystem::Relative(assetMetadata->filepath, relativeDirectoryPath);
		assetMetadata->filepath = newRelativeDirectoryPath / directoryRelativeAssetPath;
	}

	Filesystem::Rename(absoluteDirectoryPath, newName);
}

void EditorAssetManager::RenameAsset(Volt::AssetHandle assetHandle, const String& newName)
{
	Volt::WriteableAssetMetadata assetMetadata = m_referencedAssetManager.GetWriteableAssetMetadata(assetHandle);
	if (!assetMetadata.IsValid())
	{
		VT_LOGC(Warning, LogEditorAssetSystem, "Unable to rename asset with invalid ID {}!", assetHandle);
		return;
	}

	Filesystem::Path prevAbsolutePath = m_referencedAssetManager.GetAssetFilesystemPath(assetMetadata->filepath);
	assetMetadata->filepath.ReplaceFilename(newName + assetMetadata->filepath.Extension().ToString());

	Filesystem::Rename(prevAbsolutePath, newName);
}

void EditorAssetManager::MoveAssetTo(Volt::AssetHandle asset, const Filesystem::Path& targetDirectory)
{
	VT_ENSURE(!targetDirectory.IsEmpty());

	Volt::WriteableAssetMetadata assetMetadata = m_referencedAssetManager.GetWriteableAssetMetadata(asset);
	if (!assetMetadata.IsValid())
	{
		VT_LOGC(Warning, LogEditorAssetSystem, "Unable to move asset with invalid ID {}!", asset);
		return;
	}

	const Filesystem::Path relativeTargetDirectory = m_referencedAssetManager.GetRelativeAssetFilepath(targetDirectory);
	const Filesystem::Path newFilepath = targetDirectory / assetMetadata->filepath.Filename();

	Filesystem::MoveTo(m_referencedAssetManager.GetAssetFilesystemPath(assetMetadata->filepath), m_referencedAssetManager.GetAssetFilesystemPath(newFilepath));

	assetMetadata->filepath = newFilepath;
}

void EditorAssetManager::MoveDirectoryTo(const Filesystem::Path& srcDirectory, const Filesystem::Path& dstDirectory)
{
	VT_ENSURE(!srcDirectory.IsEmpty() && !dstDirectory.IsEmpty());

	Vector<Volt::AssetHandle> assetsToMove;

	Volt::AssetRegistryIteratorFilter filter;
	filter.includeMemoryAssets = false;
	filter.includeWithoutFilepath = false;

	m_referencedAssetManager.IterateAssetRegistryWithFilter(filter, [&](Volt::ReadOnlyAssetMetadata assetMetadata)
	{
		if (Filesystem::IsFilepathInDirectory(srcDirectory, assetMetadata->filepath, true))
		{
			assetsToMove.emplace_back(assetMetadata->handle);
		}

		return true;
	});

	for (const Volt::AssetHandle& assetHandle : assetsToMove)
	{
		Volt::WriteableAssetMetadata assetMetadata = m_referencedAssetManager.GetWriteableAssetMetadata(assetHandle);

		Filesystem::Path srcFilepath = m_referencedAssetManager.GetAssetFilesystemPath(assetMetadata->filepath);
		Filesystem::Path srcDirRelativePath = Filesystem::Relative(assetMetadata->filepath, srcDirectory);

		assetMetadata->filepath = dstDirectory / srcDirRelativePath;

		Filesystem::MoveTo(srcFilepath, dstDirectory / srcDirRelativePath.ParentPath());
	}
}

void EditorAssetManager::DeleteAsset(Volt::AssetHandle asset)
{
	if (!m_referencedAssetManager.IsValidAssetHandle(asset))
	{
		VT_LOGC(Warning, LogEditorAssetSystem, "Asset with handle {} is not a valid asset!", asset);
		return;
	}

	Filesystem::Path assetFilepath;
	{
		Volt::ReadOnlyAssetMetadata assetMetadata = m_referencedAssetManager.GetReadOnlyAssetMetadata(asset);
		assetFilepath = m_referencedAssetManager.GetAssetFilesystemPath(assetMetadata->filepath);
	}

	m_referencedAssetManager.RemoveAsset(asset);

	if (Filesystem::Exists(assetFilepath))
	{
		Filesystem::MoveToRecycleBin(assetFilepath);
	}

	VT_LOGC(Trace, LogEditorAssetSystem, "Deleted asset {} (Handle: {})!", assetFilepath, asset);
}

void EditorAssetManager::DeleteDirectory(const Filesystem::Path& directoryPath)
{
	Vector<Volt::AssetHandle> assetsToDelete;

	Volt::AssetRegistryIteratorFilter filter;
	filter.includeMemoryAssets = false;
	filter.includeWithoutFilepath = false;

	Filesystem::Path relativeDirectoryPath = m_referencedAssetManager.GetRelativeAssetFilepath(directoryPath);

	m_referencedAssetManager.IterateAssetRegistryWithFilter(filter, [&](Volt::ReadOnlyAssetMetadata assetMetadata) 
	{
		if (Filesystem::IsFilepathInDirectory(directoryPath, assetMetadata->filepath, true))
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
	const Filesystem::Path absoluteDirectory = m_referencedAssetManager.GetAssetFilesystemPath(relativeDirectoryPath);
	if (Filesystem::Exists(absoluteDirectory))
	{
		Filesystem::MoveToRecycleBin(absoluteDirectory);
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

void EditorAssetCache::AddAsset(IntRef<Volt::Asset> asset)
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

IntRef<Volt::Asset> EditorAssetCache::GetAsset(Volt::AssetHandle assetHandle)
{
	VT_ENSURE(assetHandle != Volt::Asset::Null());

	uint64_t hashIndex;
	if (m_hashTable.Get(assetHandle, hashIndex))
	{
		return m_cache[hashIndex];
	}

	return nullptr;
}

bool EditorAssetCache::TryGetAsset(Volt::AssetHandle assetHandle, IntRef<Volt::Asset>& outAsset)
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
