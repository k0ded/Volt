#pragma once

#include <AssetSystem/AssetManager_New.h>

VT_DECLARE_LOG_CATEGORY(LogEditorAssetSystem, LogVerbosity::Trace);

class EditorAssetManager
{
public:
	EditorAssetManager(Volt::AssetManager_New& referencedAssetManager);
	~EditorAssetManager();

	// Will return the requested asset if loaded, will otherwise stall until the asset has been loaded, will also cache the asset.
	template<Volt::VoltAssetType T> AssetReference<T> GetAssetImmediatelyAndCache(Volt::AssetHandle assetHandle);
	template<Volt::VoltAssetType T> bool TryGetAssetImmediatelyAndCache(Volt::AssetHandle assetHandle, AssetReference<T>& outAsset);

	// Will return true and the asset if it is loaded, if the asset is not loaded it will queue it for loading, and cache the asset.
	template<Volt::VoltAssetType T> bool TryGetAssetAndCache(Volt::AssetHandle assetHandle, AssetReference<T>& outAsset);

	// Will remove the asset from the cache, the asset will unload when it's no longer
	// referenced.
	void RemoveAssetFromCache(Volt::AssetHandle assetHandle);

	void RenameDirectory(const std::filesystem::path& directoryPath, const std::string& newName);
	void RenameAsset(Volt::AssetHandle assetHandle, const std::string& newName);
	void MoveAssetTo(Volt::AssetHandle asset, const std::filesystem::path& targetDirectory);
	void MoveDirectoryTo(const std::filesystem::path& srcDirectory, const std::filesystem::path& dstDirectory);
	void DeleteAsset(Volt::AssetHandle asset);
	void DeleteDirectory(const std::filesystem::path& directoryPath);

private:
	Volt::AssetManager_New& m_referencedAssetManager;
	Volt::AssetCache m_assetCache;
};

template<Volt::VoltAssetType T>
inline AssetReference<T> EditorAssetManager::GetAssetImmediatelyAndCache(Volt::AssetHandle assetHandle)
{
	RefPtr<Volt::Asset_New> asset;
	if (m_assetCache.TryGetAsset(assetHandle, asset))
	{
		return { asset };
	}

	AssetReference<T> assetReference = m_referencedAssetManager.GetAssetImmediately(assetHandle);
	m_assetCache.AddAsset(asset.GetRaw());

	return assetReference;
}

template<Volt::VoltAssetType T>
inline bool EditorAssetManager::TryGetAssetImmediatelyAndCache(Volt::AssetHandle assetHandle, AssetReference<T>& outAsset)
{
	outAsset = GetAssetImmediatelyAndCache<T>(assetHandle);
	return outAsset.IsValid();
}

template<Volt::VoltAssetType T>
inline bool EditorAssetManager::TryGetAssetAndCache(Volt::AssetHandle assetHandle, AssetReference<T>& outAsset)
{
	RefPtr<T> asset;
	if (m_assetCache.TryGetAsset(assetHandle, asset))
	{
		outAsset = { asset };
		return true;
	}

	bool loaded = m_referencedAssetManager.TryGetAsset<T>(assetHandle, outAsset);
	m_assetCache.AddAsset(outAsset.GetRaw());

	return loaded;
}

extern Scope<EditorAssetManager> g_editorAssetManager;
