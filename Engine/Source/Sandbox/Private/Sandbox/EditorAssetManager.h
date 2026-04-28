#pragma once

#include <AssetSystem/AssetManager.h>

VT_DECLARE_LOG_CATEGORY(LogEditorAssetSystem, LogVerbosity::Trace);

class EditorAssetCache
{
public:
	EditorAssetCache();
	~EditorAssetCache();

	void Clear();

	void AddAsset(IntRef<Volt::Asset> asset);
	void RemoveAsset(Volt::AssetHandle assetHandle);

	IntRef<Volt::Asset> GetAsset(Volt::AssetHandle assetHandle);
	bool TryGetAsset(Volt::AssetHandle assetHandle, IntRef<Volt::Asset>& outAsset);

private:
	void Initialize();

	AtomicHashTable<IntRef<Volt::Asset>> m_hashTable;
};

class EditorAssetManager
{
public:
	EditorAssetManager(Volt::AssetManager& referencedAssetManager);
	~EditorAssetManager();

	// Will return the requested asset if loaded, will otherwise stall until the asset has been loaded, will also cache the asset.
	template<Volt::VoltAssetType T> AssetReference<T> GetAssetImmediatelyAndCache(Volt::AssetHandle assetHandle);
	template<Volt::VoltAssetType T> bool TryGetAssetImmediatelyAndCache(Volt::AssetHandle assetHandle, AssetReference<T>& outAsset);
	template<Volt::VoltAssetType T> bool TryGetAssetImmediatelyAndCache(const Filesystem::Path& assetFilepath, AssetReference<T>& outAsset);

	// Will return true and the asset if it is loaded, if the asset is not loaded it will queue it for loading, and cache the asset.
	template<Volt::VoltAssetType T> bool TryGetAssetAndCache(Volt::AssetHandle assetHandle, AssetReference<T>& outAsset);

	// Will remove the asset from the cache, the asset will unload when it's no longer
	// referenced.
	void RemoveAssetFromCache(Volt::AssetHandle assetHandle);

	void RenameDirectory(const Filesystem::Path& directoryPath, const String& newName);
	void RenameAsset(Volt::AssetHandle assetHandle, const String& newName);
	void MoveAssetTo(Volt::AssetHandle asset, const Filesystem::Path& targetDirectory);
	void MoveDirectoryTo(const Filesystem::Path& srcDirectory, const Filesystem::Path& dstDirectory);
	void DeleteAsset(Volt::AssetHandle asset);
	void DeleteDirectory(const Filesystem::Path& directoryPath);

private:
	Volt::AssetManager& m_referencedAssetManager;
	EditorAssetCache m_assetCache;
};

template<Volt::VoltAssetType T>
inline AssetReference<T> EditorAssetManager::GetAssetImmediatelyAndCache(Volt::AssetHandle assetHandle)
{
	IntRef<Volt::Asset> asset;
	if (m_assetCache.TryGetAsset(assetHandle, asset))
	{
		return { asset.As<T>() };
	}

	AssetReference<T> assetReference = m_referencedAssetManager.GetAssetImmediately<T>(assetHandle);
	if (assetReference.IsValid())
	{
		m_assetCache.AddAsset(assetReference.GetRaw());
	}

	return assetReference;
}

template<Volt::VoltAssetType T>
inline bool EditorAssetManager::TryGetAssetImmediatelyAndCache(Volt::AssetHandle assetHandle, AssetReference<T>& outAsset)
{
	outAsset = GetAssetImmediatelyAndCache<T>(assetHandle);
	return outAsset.IsValid();
}

template<Volt::VoltAssetType T>
bool EditorAssetManager::TryGetAssetImmediatelyAndCache(const Filesystem::Path& assetFilepath, AssetReference<T>& outAsset)
{
	Volt::AssetHandle assetHandle = m_referencedAssetManager.GetAssetHandleFromFilepath(assetFilepath);
	if (assetHandle != Volt::Asset::Null())
	{
		outAsset = GetAssetImmediatelyAndCache<T>(assetHandle);
	}

	return outAsset.IsValid();
}

template<Volt::VoltAssetType T>
inline bool EditorAssetManager::TryGetAssetAndCache(Volt::AssetHandle assetHandle, AssetReference<T>& outAsset)
{
	IntRef<Volt::Asset> asset;
	if (m_assetCache.TryGetAsset(assetHandle, asset))
	{
		outAsset = { asset.As<T>() };

		Volt::ReadOnlyAssetMetadata metadata = m_referencedAssetManager.GetReadOnlyAssetMetadata(assetHandle);
		return metadata->IsLoaded();
	}

	bool loaded = m_referencedAssetManager.TryGetAsset<T>(assetHandle, outAsset);
	m_assetCache.AddAsset(outAsset.GetRaw());

	return loaded;
}

extern Unique<EditorAssetManager> g_editorAssetManager;
