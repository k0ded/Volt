#include "aspch.h"
#include "AssetManager_New.h"

namespace Volt
{
	Scope<AssetManager_New> g_assetManager;

	AssetManager_New::AssetManager_New(const std::filesystem::path& engineDirectoryPath, const std::filesystem::path& projectDirectoryPath, std::string_view assetsDirectoryName)
		: m_assetRegistry(engineDirectoryPath, projectDirectoryPath, assetsDirectoryName)
	{}

	AssetManager_New::~AssetManager_New()
	{}

	WriteableAssetMetadata AssetManager_New::GetWriteableAssetMetadata(AssetHandle assetHandle) const
	{
		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		return { assetMetadata};
	}

	ReadOnlyAssetMetadata AssetManager_New::GetReadOnlyAssetMetadata(AssetHandle assetHandle) const
	{
		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		return { assetMetadata };
	}

	AssetMetadata AssetManager_New::GetAssetMetadataCopy(AssetHandle assetHandle) const
	{
		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		if (assetMetadata)
		{
			return *assetMetadata;
		}

		return {};
	}
	
	void AssetManager_New::ReloadAsset(AssetHandle assetHandle)
	{
		RefPtr<Asset_New> asset = m_assetCache.GetAsset(assetHandle);
		ScopedAssetLock assetLock(asset);

	
	}

	void AssetManager_New::SaveAsset(AssetHandle assetHandle)
	{

	}

	bool AssetManager_New::IsValidAssetHandle(AssetHandle assetHandle) const
	{
		return m_assetRegistry.IsValidAssetHandle(assetHandle);
	}

	void AssetManager_New::CreateFileForAsset(AssetHandle assetHandle, const std::filesystem::path& filepath)
	{

	}

	AssetManager_New::AssetUpdatedCallbackID AssetManager_New::RegisterAssetUpdatedCallback(AssetType assetType, AssetChangedCallback&& callback)
	{
		return {};
	}

	void AssetManager_New::AddDependencyToAsset(AssetHandle dependant, AssetHandle dependency)
	{

	}

	Vector<AssetHandle> AssetManager_New::GetAssetsDependentOn(AssetHandle assetHandle) const
	{
		return {};
	}

	void AssetManager_New::UnloadAndFreeAsset(AssetRefCounter* assetRefCounter)
	{
		// Safe to upcast like this, because AssetRefCounter should only be derived by Asset.
		Asset_New* asset = reinterpret_cast<Asset_New*>(assetRefCounter);
		const AssetHandle assetHandle = asset->GetAssetHandle();

		bool isMemoryAsset = false;

		// Make sure we lock the metadata
		{
			WriteableAssetMetadata assetMetadata = GetWriteableAssetMetadata(assetHandle);

			m_assetCache.RemoveAsset(assetHandle);

			// At this point there should be zero references left.
			VT_ENSURE(asset->GetRefCount() == 0);

			// Call destructor and free.
			const AssetType assetType = asset->GetType();

			asset->~Asset_New();
			m_assetAllocator.FreeAsset(assetType, asset);

			if (!assetMetadata->isMemoryAsset)
			{
				assetMetadata->isLoaded = false;
				assetMetadata->isQueued = false;
			}
			else
			{
				isMemoryAsset = true;
			}
		}

		// If the asset is a memory asset, we will also remove it from the registry.
		// There is no reason to keep it around.
		// Make sure to remove the metadata after the lock has been released.
		if (isMemoryAsset)
		{
			m_assetRegistry.RemoveAssetMetadata(assetHandle);
		}
	}

	AssetManager_New::ScopedAssetLock::ScopedAssetLock(RefPtr<Asset_New> asset)
		: m_asset(asset)
	{
		m_asset->m_assetReferenceLock.wait(0, std::memory_order::acquire);
		VT_ENSURE_MSG(m_asset->m_assetManagerLock.test(std::memory_order::relaxed) == false, "An asset should only be locked once at a time!");
		
		m_asset->m_assetManagerLock.test_and_set(std::memory_order::acquire);
	}

	AssetManager_New::ScopedAssetLock::~ScopedAssetLock()
	{
		m_asset->m_assetManagerLock.clear(std::memory_order::release);
		m_asset->m_assetManagerLock.notify_all();
	}
}
