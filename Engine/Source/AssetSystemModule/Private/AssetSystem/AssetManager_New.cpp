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

	LockedAssetMetadata AssetManager_New::GetWriteableAssetMetadata(AssetHandle assetHandle) const
	{
		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		return { assetMetadata};
	}

	AssetMetadataConstReference AssetManager_New::GetReadOnlyAssetMetadata(AssetHandle assetHandle) const
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

	}

	void AssetManager_New::SaveAsset(AssetHandle assetHandle)
	{

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
		m_assetCache.RemoveAsset(asset->GetAssetHandle());

		// At this point there should be zero references left.
		VT_ENSURE(asset->GetRefCount() == 0);

		// Call destructor and free.
		const AssetType assetType = asset->GetType();

		asset->~Asset_New();
		m_assetAllocator.FreeAsset(assetType, asset);
	}
}
