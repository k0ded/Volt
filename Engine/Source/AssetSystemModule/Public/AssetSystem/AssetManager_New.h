#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetRegistry.h"
#include "AssetSystem/AssetAllocator.h"
#include "AssetSystem/AssetCache.h"

#include <CoreUtilities/Pointers/RefPtr.h>

#include <filesystem>

namespace Volt
{
	// A wrapper of the asset metadata that ensures no other
	// thread can access it at the same time.
	// Takes a asset metadata as a reference, it is ok as all metadata
	// has stable pointers.
	class LockedAssetMetadata
	{
	public:
		LockedAssetMetadata(AssetMetadata* inMetadata)
			: m_metadata(inMetadata)
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.lock();
			}
		}

		~LockedAssetMetadata()
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.unlock();
			}
		}

		AssetMetadata* operator->() noexcept
		{
			return m_metadata;
		}

	private:
		AssetMetadata* m_metadata;
	};

	class AssetMetadataConstReference
	{
	public:
		AssetMetadataConstReference(AssetMetadata* inMetadata)
			: m_metadata(inMetadata)
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.lock_shared();
			}
		}

		~AssetMetadataConstReference()
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.unlock_shared();
			}
		}

		const AssetMetadata* operator->() const noexcept
		{
			return m_metadata;
		}

	private:
		AssetMetadata* m_metadata;
	};

	class VTAS_API AssetManager_New
	{
	public:
		using AssetUpdatedCallbackID = UUID64;
		using AssetChangedCallback = std::function<void(AssetHandle assetHandle, AssetChangedState state)>;

		AssetManager_New(const std::filesystem::path& engineDirectoryPath, const std::filesystem::path& projectDirectoryPath, std::string_view assetsDirectoryName);
		~AssetManager_New();

		///// Asset Metadata /////
		LockedAssetMetadata GetWriteableAssetMetadata(AssetHandle assetHandle) const;
		AssetMetadataConstReference GetReadOnlyAssetMetadata(AssetHandle assetHandle) const;
		AssetMetadata GetAssetMetadataCopy(AssetHandle assetHandle) const;

		///// Asset /////
		// Will unload and load the asset again, the pointer to the asset will remain the same,
		// to allow other systems to keep a reference to the asset.
		void ReloadAsset(AssetHandle assetHandle);

		// Will trigger a serialization of the asset, if it has an assigned filepath.
		void SaveAsset(AssetHandle assetHandle);
		
		// Will return the requested asset if loaded, will otherwise stall until the asset has been loaded.
		template<VoltAssetType T> RefPtr<T> GetAssetImmediately(AssetHandle assetHandle);
		// Will return true and the asset if it is loaded, if the asset is not loaded it will queue it for loading.
		template<VoltAssetType T> bool TryGetAsset(AssetHandle assetHandle, RefPtr<T>& outAsset);

		// Creates an asset that only lives in memory during the current application run, is not serializable to disk.
		template<VoltAssetType T, typename... Args> RefPtr<T> CreateMemoryAsset(std::string_view assetName, Args&&... args);
		// Creates an asset that does not have a filepath yet.
		template<VoltAssetType T, typename... Args> RefPtr<T> CreateAsset(std::string_view assetName, Args&&... args);
		// Creates an asset, assigns a filepath and creates the asset disk file itself.
		template<VoltAssetType T, typename... Args> RefPtr<T> CreateAssetAndFile(const std::filesystem::path& targetDirectory, std::string_view assetName, Args&&... args);

		void CreateFileForAsset(AssetHandle assetHandle, const std::filesystem::path& filepath);

		///// Management /////
		AssetUpdatedCallbackID RegisterAssetUpdatedCallback(AssetType assetType, AssetChangedCallback&& callback);
		void UnregisterAssetUpdatedCallback(AssetType assetType, UUID64 callbackId);

		void AddDependencyToAsset(AssetHandle dependant, AssetHandle dependency);
		Vector<AssetHandle> GetAssetsDependentOn(AssetHandle assetHandle) const;

	private:
		friend class AssetRefCounter;

		template<VoltAssetType T, typename... Args> RefPtr<T> CreateAssetImpl(std::string_view assetName, bool isMemoryAsset, Args&&... args);

		void UnloadAndFreeAsset(AssetRefCounter* assetRefCounter);

		AssetRegistry m_assetRegistry;
		AssetAllocator m_assetAllocator;
		AssetCache m_assetCache;
	};

	template<VoltAssetType T> 
	RefPtr<T> AssetManager_New::GetAssetImmediately(AssetHandle assetHandle)
	{

	}

	template<VoltAssetType T>
	bool AssetManager_New::TryGetAsset(AssetHandle assetHandle, RefPtr<T>& outAsset)
	{

	}

	template<VoltAssetType T, typename... Args> 
	RefPtr<T> AssetManager_New::CreateMemoryAsset(std::string_view assetName, Args&&... args)
	{
		constexpr bool IsMemoryAsset = true;
		return CreateAssetImpl<T>(assetName, IsMemoryAsset, std::forward<Args>(args)...);
	}

	template<VoltAssetType T, typename... Args> 
	RefPtr<T> AssetManager_New::CreateAsset(std::string_view assetName, Args&&... args)
	{
		constexpr bool IsMemoryAsset = false;
		return CreateAssetImpl<T>(assetName, IsMemoryAsset, std::forward<Args>(args)...);
	}

	template<VoltAssetType T, typename... Args> 
	RefPtr<T> AssetManager_New::CreateAssetAndFile(const std::filesystem::path& targetDirectory, std::string_view assetName, Args&&... args)
	{

	}

	template<VoltAssetType T, typename... Args> 
	RefPtr<T> AssetManager_New::CreateAssetImpl(std::string_view assetName, bool isMemoryAsset, Args&&... args)
	{
		RefPtr<T> newAsset = m_assetAllocator.AllocateAsset<T>(std::forward<Args>(args)...);

		AssetMetadata metadata{};
		metadata.filePath = ""; // Assets that are not saved will not have a file path
		metadata.handle = newAsset->GetAssetHandle();
		metadata.type = T::GetStaticType();
		metadata.isLoaded = true;
		metadata.isMemoryAsset = isMemoryAsset;

		newAsset->SetupInitialCustomMetadata(metadata.customData);
		newAsset->SetName(std::string(assetName));

		// Setup a link back to the asset manager.
		AssetRefCounter* assetRefCounter = static_cast<AssetRefCounter*>(newAsset.GetRaw());
		assetRefCounter->m_referencedAssetManager = this;

		m_assetRegistry.InsertAssetMetadata(std::move(metadata));

		m_assetCache.AddAsset(newAsset);

		return newAsset;
	}

	VTAS_API extern Scope<AssetManager_New> g_assetManager;
}
