#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetRegistry.h"
#include "AssetSystem/AssetAllocator.h"
#include "AssetSystem/AssetCache.h"
#include "AssetSystem/AssetReference.h"
#include "AssetSystem/AssetSerializerRegistry.h"
#include "AssetSystem/AssetDependencyGraph.h"

#include <LogModule/Log.h>

#include <CoreUtilities/Pointers/RefPtr.h> 
#include <CoreUtilities/WorkQueue.h>

#include <filesystem>

namespace Volt
{
	VT_DECLARE_LOG_CATEGORY_EXPORT(VTAS_API, LogAssetSystem, LogVerbosity::Trace);

	class AssetManager_New
	{
	public:
		using AssetUpdatedCallbackID = UUID64;
		using AssetChangedCallback = std::function<void(AssetHandle assetHandle, AssetChangedState state)>;
		using AssetRegistryIteratorFunc = std::function<bool(ReadOnlyAssetMetadata)>;

		VTAS_API AssetManager_New(const std::filesystem::path& engineDirectoryPath, const std::filesystem::path& projectDirectoryPath, std::string_view assetsDirectoryName);
		VTAS_API ~AssetManager_New();

		///// Asset Metadata /////
		VTAS_API WriteableAssetMetadata GetWriteableAssetMetadata(AssetHandle assetHandle) const;
		VTAS_API ReadOnlyAssetMetadata GetReadOnlyAssetMetadata(AssetHandle assetHandle) const;
		VTAS_API AssetMetadata GetAssetMetadataCopy(AssetHandle assetHandle) const;

		///// Asset /////
		// Will unload and load the asset again, the pointer to the asset will remain the same,
		// to allow other systems to keep a reference to the asset.
		VTAS_API void ReloadAsset(AssetHandle assetHandle);

		// Will trigger a serialization of the asset, if it has an assigned filepath.
		VTAS_API void SaveAsset(AssetHandle assetHandle);
		VTAS_API void SaveAsset(AssetReference<Asset_New> asset);

		// Will remove the asset from the asset registry, asset cache, and will send out
		// deleted events. The asset is not guaranteed to be destroyed immediatley, as there
		// may still be other references. This will NOT remove the asset from disk.
		VTAS_API void RemoveAsset(AssetHandle assetHandle);

		// Returns whether or not the asset exists in the asset registry.
		VTAS_API bool IsValidAssetHandle(AssetHandle assetHandle) const;

		// Returns whether or not the asset is loaded.
		VTAS_API bool IsAssetLoaded(AssetHandle assetHandle) const;

		// Returns the asset handle corresponding to the asset file.
		// Has to be a relative file path.
		VTAS_API AssetHandle GetAssetHandleFromFilepath(const std::filesystem::path& filepath) const;

		// Will return the requested asset if loaded, will otherwise stall until the asset has been loaded.
		template<VoltAssetType T> AssetReference<T> GetAssetImmediately(AssetHandle assetHandle);
		template<VoltAssetType T> AssetReference<T> GetAssetImmediately(const std::filesystem::path& assetFilepath);
		template<VoltAssetType T> bool TryGetAssetImmediately(AssetHandle assetHandle, AssetReference<T>& outAsset);
		template<VoltAssetType T> bool TryGetAssetImmediately(const std::filesystem::path& assetFilepath, AssetReference<T>& outAsset);

		// Will return true and the asset if it is loaded, if the asset is not loaded it will queue it for loading.
		template<VoltAssetType T> bool TryGetAsset(AssetHandle assetHandle, AssetReference<T>& outAsset);

		// Will return true and the asset if is is loaded, otherwise it will return false.
		// This will NOT queue the asset for loading.
		template<VoltAssetType T> bool TryGetAssetIfLoaded(AssetHandle assetHandle, AssetReference<T>& outAsset);

		// Will return true and the asset if it is loaded. This method returns a non typed asset,
		// instead of the default typed asset.
		VTAS_API bool TryGetAssetIfLoadedAsAnonymous(AssetHandle assetHandle, AssetReference<Asset_New>& outAsset);

		// Creates an asset that only lives in memory during the current application run, is not serializable to disk.
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateMemoryAsset(std::string_view assetName, Args&&... args);
		// Creates an asset that does not have a filepath yet.
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAsset(std::string_view assetName, Args&&... args);
		// Creates an asset, assigns a filepath and creates the asset disk file itself.
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAssetAndFile(const std::filesystem::path& targetDirectory, std::string_view assetName, Args&&... args);

		VTAS_API void CreateFileForAsset(AssetHandle assetHandle, const std::filesystem::path& filepath);

		///// Management /////
		VTAS_API AssetUpdatedCallbackID RegisterAssetUpdatedCallback(AssetType assetType, AssetChangedCallback&& callback);
		VTAS_API void UnregisterAssetUpdatedCallback(AssetType assetType, UUID64 callbackId);

		VTAS_API void AddDependencyToAsset(AssetHandle dependant, AssetHandle dependency);
		VTAS_API Vector<AssetHandle> GetAssetsDependentOn(AssetHandle assetHandle) const;

		void QueueAssetChanged(AssetHandle assetHandle, AssetChangedState state);

		///// Asset Registry /////
		// Iterates the asset registry with a filter, return false to exit the loop.
		VTAS_API void IterateAssetRegistryWithFilter(const AssetRegistryIteratorFilter& filter, AssetRegistryIteratorFunc&& func) const;

		///// File System /////
		VTAS_API std::filesystem::path GetContextPath(const std::filesystem::path& path) const;
		VTAS_API std::filesystem::path GetFilesystemPath(const std::filesystem::path& path) const;
		VTAS_API std::filesystem::path GetFilesystemPath(AssetHandle assetHandle) const;
		VTAS_API std::filesystem::path GetRelativeAssetFilepath(const std::filesystem::path& path) const;
		VTAS_API bool IsEngineAsset(const std::filesystem::path& path) const;

	private:
		friend class AssetRefCounter;

		struct ScopedAssetLock
		{
			ScopedAssetLock(RefPtr<Asset_New> asset);
			~ScopedAssetLock();

		private:
			RefPtr<Asset_New> m_asset;
		};

		struct AssetManagerRoot
		{
			std::filesystem::path engineDirectoryPath;
			std::filesystem::path projectDirectoryPath;
			std::string_view assetsDirectoryName;
		};

		struct AssetChangedQueueInfo
		{
			AssetHandle handle;
			AssetChangedState state;
		};

		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAssetImpl(std::string_view assetName, bool isMemoryAsset, Args&&... args);

		VTAS_API void LoadAsset(AssetHandle assetHandle, RefPtr<Asset_New> asset);
		void UnloadAndFreeAsset(AssetRefCounter* assetRefCounter);

		void CreateDependencyGraphAndAddAssetsFromRegistry();
		ReadOnlyAssetMetadata GetAssetMetadataFromFilepath(const std::filesystem::path& filepath);

		AssetRegistry m_assetRegistry;
		AssetAllocator m_assetAllocator;
		AssetCache m_assetCache;

		Scope<AssetDependencyGraph> m_dependencyGraph;
		WorkQueue<AssetChangedQueueInfo, QueueThreadingPolicy::MPSC> m_assetChangedQueue;
		AssetManagerRoot m_root;
	};

	template<VoltAssetType T> 
	AssetReference<T> AssetManager_New::GetAssetImmediately(AssetHandle assetHandle)
	{
		// Make sure the asset exists.
		if (!m_assetRegistry.IsValidAssetHandle(assetHandle))
		{
			VT_LOGC(Warning, LogAssetSystem, "Asset handle {} is not a valid asset handle!", assetHandle);
			return {};
		}

		// Try to get the asset from the asset cache.
		RefPtr<Asset_New> tempAsset;
		if (m_assetCache.TryGetAsset(assetHandle, tempAsset))
		{
			VT_ENSURE(T::GetStaticType() == tempAsset->GetType());
			return AssetReference<T>(tempAsset.As<T>());
		}

		// Check if we can actually load this asset.
		if (!AssetSerializerRegistry::Get().HasSerializer(T::GetStaticType()))
		{
			VT_LOGC(Warning, LogAssetSystem, "No serializer for asset {} with type {} was found!", assetHandle, T::GetStaticType()->GetName());
			return {};
		}

		// Asset wasn't in the cache, create and load it.
		RefPtr<T> newAsset = m_assetAllocator.AllocateAsset<T>();
		// Setup a link back to the asset manager.
		newAsset->m_referencedAssetManager = this;

		AssetReference resultReference{ newAsset };

		{
			ReadOnlyAssetMetadata metadata = m_assetRegistry.GetAssetMetadata(assetHandle);

			// All metadatas should be valid.
			VT_ENSURE(metadata->IsValid());

			newAsset->AssignAssetHandle(metadata->handle);
			newAsset->SetName(metadata->filepath.stem().string());
		}

		LoadAsset(assetHandle, newAsset);

		return resultReference;
	}

	template<VoltAssetType T> AssetReference<T>
	AssetManager_New::GetAssetImmediately(const std::filesystem::path& assetFilepath)
	{
		AssetHandle assetHandle = GetAssetHandleFromFilepath(assetFilepath);
		if (assetHandle != Asset_New::Null())
		{
			return GetAssetImmediately<T>(assetHandle);
		}
		
		VT_LOGC(Error, LogAssetSystem, "Unable to load asset from filepath '{}'!", assetFilepath);
		return {};
	}

	template<VoltAssetType T>
	bool AssetManager_New::TryGetAssetImmediately(AssetHandle assetHandle, AssetReference<T>& outAsset)
	{
		outAsset = GetAssetImmediately<T>(assetHandle);
		return outAsset.IsValid();
	}

	template<VoltAssetType T>
	bool AssetManager_New::TryGetAssetImmediately(const std::filesystem::path& assetFilepath, AssetReference<T>& outAsset)
	{
		AssetHandle assetHandle = GetAssetHandleFromFilepath(assetFilepath);
		if (assetHandle != Asset_New::Null())
		{
			outAsset = GetAssetImmediately<T>(assetHandle);
		}
		else
		{
			outAsset = {};
			VT_LOGC(Error, LogAssetSystem, "Unable to load asset from filepath '{}'!", assetFilepath);
		}

		return outAsset.IsValid();
	}

	template<VoltAssetType T>
	bool AssetManager_New::TryGetAsset(AssetHandle assetHandle, AssetReference<T>& outAsset)
	{
		// #TODO_AssetSystem: Implement queueing.
		return TryGetAssetImmediately(assetHandle, outAsset);
	}

	template<VoltAssetType T>
	bool AssetManager_New::TryGetAssetIfLoaded(AssetHandle assetHandle, AssetReference<T>& outAsset)
	{
		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(assetHandle);
		if (assetMetadata->isLoaded)
		{
			outAsset = GetAssetImmediately<T>(assetHandle);
			return true;
		}

		return false;
	}

	template<VoltAssetType T, typename... Args>
	AssetReference<T> AssetManager_New::CreateMemoryAsset(std::string_view assetName, Args&&... args)
	{
		constexpr bool IsMemoryAsset = true;
		return CreateAssetImpl<T>(assetName, IsMemoryAsset, std::forward<Args>(args)...);
	}

	template<VoltAssetType T, typename... Args> 
	AssetReference<T> AssetManager_New::CreateAsset(std::string_view assetName, Args&&... args)
	{
		constexpr bool IsMemoryAsset = false;
		return CreateAssetImpl<T>(assetName, IsMemoryAsset, std::forward<Args>(args)...);
	}

	template<VoltAssetType T, typename... Args> 
	AssetReference<T> AssetManager_New::CreateAssetAndFile(const std::filesystem::path& targetDirectory, std::string_view assetName, Args&&... args)
	{
		AssetReference<T> asset = CreateAsset<T>(assetName, std::forward<Args>(args)...);

		std::filesystem::path targetPath = targetDirectory / (std::string(assetName) + ".vtasset");
		CreateFileForAsset(asset->GetAssetHandle(), targetPath);

		return asset;
	}

	template<VoltAssetType T, typename... Args> 
	AssetReference<T> AssetManager_New::CreateAssetImpl(std::string_view assetName, bool isMemoryAsset, Args&&... args)
	{
		RefPtr<T> newAsset = m_assetAllocator.AllocateAsset<T>(std::forward<Args>(args)...);

		AssetMetadata metadata{};
		metadata.filepath = ""; // Assets that are not saved will not have a file path
		metadata.handle = newAsset->GetAssetHandle();
		metadata.type = T::GetStaticType();
		metadata.isLoaded = true;
		metadata.isMemoryAsset = isMemoryAsset;

		newAsset->SetupInitialCustomMetadata(metadata.customData);
		newAsset->SetName(std::string(assetName));

		if (isMemoryAsset)
		{
			newAsset->SetFlag(AssetFlag::MemoryOnly, true);
		}

		// Setup a link back to the asset manager.
		newAsset->m_referencedAssetManager = this;

		m_assetRegistry.InsertAssetMetadata(std::move(metadata));

		m_assetCache.AddAsset(newAsset);

		return newAsset;
	}
}
VTAS_API extern Scope<Volt::AssetManager_New> g_assetManager;
