#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetRegistry.h"
#include "AssetSystem/AssetAllocator.h"
#include "AssetSystem/AssetCache.h"
#include "AssetSystem/AssetReference.h"
#include "AssetSystem/AssetDependencyGraph.h"

#include <EventSystem/EventListener.h>

#include <LogModule/Log.h>

#include <CoreUtilities/Pointers/IntRef.h> 
#include <CoreUtilities/Pointers/Unique.h>
#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Locks/SpinMutex.h>

namespace Volt
{
	VT_DECLARE_LOG_CATEGORY_EXPORT(VTAS_API, LogAssetSystem, LogVerbosity::Trace);

	class AssetManager : public EventListener
	{
	public:
		inline static constexpr uint32_t AssetFileMagic = 252525;

		using AssetUpdatedCallbackID = UUID64;
		using AssetChangedCallback = std::function<void(AssetHandle assetHandle, AssetChangedState state)>;
		using AssetRegistryIteratorFunc = std::function<bool(ReadOnlyAssetMetadata)>;

		VTAS_API AssetManager(const Filesystem::Path& engineDirectoryPath, const Filesystem::Path& projectDirectoryPath, StringView assetsDirectoryName);
		VTAS_API ~AssetManager();

		VTAS_API int32_t GetNumAssetsInRegistry() const;

		///// Asset Metadata /////
		// Returns a single thread writeable accessor, locks for the
		// duration of the WriteableAssetMetadata object.
		VTAS_API WriteableAssetMetadata GetWriteableAssetMetadata(AssetHandle assetHandle) const;

		// Returns a multi thread readable accessor, locks for the
		// duration of the ReadOnlyAssetMetadata object.
		VTAS_API ReadOnlyAssetMetadata GetReadOnlyAssetMetadata(AssetHandle assetHandle) const;
		VTAS_API AssetMetadata GetAssetMetadataCopy(AssetHandle assetHandle) const;

		///// Asset /////
		// Will unload and load the asset again, the pointer to the asset will remain the same,
		// to allow other systems to keep a reference to the asset.
		VTAS_API void ReloadAsset(AssetHandle assetHandle);

		// Will trigger a serialization of the asset, if it has an assigned filepath.
		VTAS_API void SaveAsset(AssetHandle assetHandle);
		VTAS_API void SaveAsset(AssetReference<Asset> asset);

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
		VTAS_API AssetHandle GetAssetHandleFromFilepath(const Filesystem::Path& filepath) const;

		// Will return the requested asset if loaded, will otherwise stall until the asset has been loaded.
		template<VoltAssetType T> AssetReference<T> GetAssetImmediately(AssetHandle assetHandle);
		template<VoltAssetType T> AssetReference<T> GetAssetImmediately(const Filesystem::Path& assetFilepath);
		template<VoltAssetType T> bool TryGetAssetImmediately(AssetHandle assetHandle, AssetReference<T>& outAsset);
		template<VoltAssetType T> bool TryGetAssetImmediately(const Filesystem::Path& assetFilepath, AssetReference<T>& outAsset);

		// Will return true and the asset if it is loaded, if the asset is not loaded it will queue it for loading.
		// The outAsset value might be filled with a valid asset.
		template<VoltAssetType T> bool TryGetAsset(AssetHandle assetHandle, AssetReference<T>& outAsset);

		// Will return true and the asset if is is loaded, otherwise it will return false.
		// This will NOT queue the asset for loading.
		template<VoltAssetType T> bool TryGetAssetIfLoaded(AssetHandle assetHandle, AssetReference<T>& outAsset);

		// Will return true and the asset if it is loaded. This method returns a non typed asset,
		// instead of the default typed asset.
		VTAS_API bool TryGetTypelessAssetIfLoaded(AssetHandle assetHandle, AssetReference<Asset>& outAsset);
		VTAS_API bool TryGetTypelessAssetImmediately(AssetHandle assetHandle, AssetReference<Asset>& outAsset);

		// Will return true and the asset if it is loaded, if the asset is not loaded it will queue it for loading.
		// The outAsset value might be filled with a valid asset.
		VTAS_API bool TryGetTypelessAsset(AssetHandle assetHandle, AssetReference<Asset>& outAsset);

		// Creates an asset that only lives in memory during the current application run, is not serializable to disk.
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateMemoryAsset(StringView assetName, Args&&... args);
		// Creates an asset that only lives in memory, and will not show up when iterating the asset registry.
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAnonymousAsset(StringView assetName, Args&&... args);
		// Creates an asset that does not have a filepath yet.
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAsset(StringView assetName, Args&&... args);
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAssetWithAssetHandle(StringView assetName, AssetHandle assetHandle, Args&&... args);
		// Creates an asset, assigns a filepath and creates the asset disk file itself.
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAssetAndFile(const Filesystem::Path& targetDirectory, StringView assetName, Args&&... args);
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAssetAndFileWithAssetHandle(const Filesystem::Path& targetDirectory, StringView assetName, AssetHandle assetHandle, Args&&... args);

		// Creates an asset of a type without arguments.
		VTAS_API AssetReference<Asset> CreateAssetTypeless(StringView assetName, AssetType assetType);

		VTAS_API void CreateFileForAsset(AssetHandle assetHandle, const Filesystem::Path& filepath);

		///// Management /////
		VTAS_API AssetUpdatedCallbackID RegisterAssetUpdatedCallback(AssetType assetType, AssetChangedCallback&& callback);
		VTAS_API void UnregisterAssetUpdatedCallback(AssetType assetType, UUID64 callbackId);

		VTAS_API Vector<AssetHandle> GetAssetsDependentOn(AssetHandle assetHandle) const;

		///// Asset Registry /////
		// Iterates the asset registry with a filter, return false to exit the loop.
		VTAS_API void IterateAssetRegistryWithFilter(const AssetRegistryIteratorFilter& filter, AssetRegistryIteratorFunc&& func) const;

		///// File System /////
		VTAS_API Filesystem::Path GetContextPath(const Filesystem::Path& path) const;
		VTAS_API Filesystem::Path GetAssetFilesystemPath(const Filesystem::Path& path) const;
		VTAS_API Filesystem::Path GetAssetFilesystemPath(AssetHandle assetHandle) const;
		VTAS_API Filesystem::Path GetRelativeAssetFilepath(const Filesystem::Path& path) const;
		VTAS_API bool IsEngineAsset(const Filesystem::Path& path) const;

		VTAS_API JobCounterRef GetMetadataLoadingCounter();
	private:
		friend class AssetRefCounter;

		struct AssetManagerRoot
		{
			Filesystem::Path engineDirectoryPath;
			Filesystem::Path projectDirectoryPath;
			StringView assetsDirectoryName;
		};

		struct AssetChangedQueueInfo
		{
			AssetHandle handle;
			AssetChangedState state;
		};

		struct AssetChangedCallbackInfo
		{
			UUID64 id;
			AssetChangedCallback callback;
		};

		struct AssetUnloadData
		{
			AssetRefCounter* asset;
		};

		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAssetImpl(StringView assetName, bool isMemoryAsset, bool isAnonymous, AssetHandle assetHandle, Args&&... args);

		VTAS_API void LoadAsset(AssetHandle assetHandle, IntRef<Asset> asset, AssetLoadState expectedLoadState);
		VTAS_API void QueueAssetForLoading(AssetHandle assetHandle, IntRef<Asset> asset, AssetLoadState expectedLoadState);

		VTAS_API IntRef<Asset> TryCreateAsset(AssetHandle assetHandle, AssetLoadState expectedLoadState, AssetLoadState dstLoadState, bool& wasCreated);

		VTAS_API void AddAssetToCache(IntRef<Asset> asset);
		IntRef<Asset> TryGetOrTryWaitForPublishedAsset(AssetHandle assetHandle);

		void QueueAssetForEviction(AssetRefCounter* assetRefCounter);
		void RunGarbageCollection(uint64_t frameIndex, bool forceCleanupAll);

		bool DeserializeAsset(AssetReference<Asset> asset);

		bool SerializeAsset(AssetReference<Asset> asset);
		void SerializeAssetHeader(Archive& archive, AssetMetadata assetMetadata, uint32_t assetVersion);

		void OnAssetChanged(AssetHandle assetHandle, AssetChangedState state);
		VTAS_API void QueueAssetChanged(AssetHandle assetHandle, AssetChangedState state);
		bool UpdateInternal(class AppTickEvent& e);

		void CreateDependencyGraphAndAddAssetsFromRegistry();
		ReadOnlyAssetMetadata GetAssetMetadataFromFilepath(const Filesystem::Path& filepath);

		AssetRegistry m_assetRegistry;
		AssetAllocator m_assetAllocator;
		AssetCache m_assetCache;

		Unique<AssetDependencyGraph> m_dependencyGraph;
		AssetManagerRoot m_root;

		// May be read from multiple threads.
		std::atomic<uint64_t> m_frameIndex = 0;

		// Asset changes callbacks
		WorkQueue<AssetChangedQueueInfo, QueueThreadingPolicy::MPSC> m_assetChangedQueue;
		WorkQueue<AssetUnloadData, QueueThreadingPolicy::MPSC> m_assetDestructionQueue;

		// Eviction
		struct EvictionEntry
		{
			Asset* asset;
			AssetCache::Container* cacheContainer;
			uint64_t evictedOnFrameIndex;
		};

		struct AssetMetadataReclamationEntry
		{
			AssetMetadata* assetMetadata;
			uint64_t evictedOnFrameIndex;
		};

		WorkQueue<AssetRefCounter*, QueueThreadingPolicy::MPSC> m_assetEvictionQueue;
		Vector<EvictionEntry> m_assetReclamationList;

		SpinMutex m_assetMetadataReclamationMutex;
		Vector<AssetMetadataReclamationEntry> m_assetMetadataReclamationList;

		std::mutex m_assetCallbackMutex;
		Map<AssetType, Vector<AssetChangedCallbackInfo>> m_assetChangedCallbacks;
	};

	template<VoltAssetType T> 
	AssetReference<T> AssetManager::GetAssetImmediately(AssetHandle assetHandle)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		// Make sure the asset exists.
		if (!m_assetRegistry.IsValidAssetHandle(assetHandle))
		{
			VT_LOGC(Warning, LogAssetSystem, "Asset handle '{}' is not a valid asset handle!", assetHandle);
			return {};
		}

		bool wasCreated = false;
		IntRef<Asset> newAsset = TryCreateAsset(assetHandle, AssetLoadState::Unloaded, AssetLoadState::Loading, wasCreated);

		if (wasCreated)
		{
			// The asset was created by this thread, let's load it.
			LoadAsset(assetHandle, newAsset, AssetLoadState::Loading);
		}

		return newAsset.As<T>();
	}

	template<VoltAssetType T> 
	AssetReference<T> AssetManager::GetAssetImmediately(const Filesystem::Path& assetFilepath)
	{
		AssetHandle assetHandle = GetAssetHandleFromFilepath(assetFilepath);
		if (assetHandle != Asset::Null())
		{
			return GetAssetImmediately<T>(assetHandle);
		}
		
		VT_LOGC(Error, LogAssetSystem, "Unable to load asset from filepath '{}'!", assetFilepath);
		return {};
	}

	template<VoltAssetType T>
	bool AssetManager::TryGetAssetImmediately(AssetHandle assetHandle, AssetReference<T>& outAsset)
	{
		outAsset = GetAssetImmediately<T>(assetHandle);
		return outAsset.IsValid();
	}

	template<VoltAssetType T>
	bool AssetManager::TryGetAssetImmediately(const Filesystem::Path& assetFilepath, AssetReference<T>& outAsset)
	{
		AssetHandle assetHandle = GetAssetHandleFromFilepath(assetFilepath);
		if (assetHandle != Asset::Null())
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
	bool AssetManager::TryGetAsset(AssetHandle assetHandle, AssetReference<T>& outAsset)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		// Make sure the asset exists.
		if (!m_assetRegistry.IsValidAssetHandle(assetHandle))
		{
			VT_LOGC(Warning, LogAssetSystem, "Asset handle '{}' is not a valid asset handle!", assetHandle);
			return false;
		}

		bool wasCreated = false;
		IntRef<Asset> newAsset = TryCreateAsset(assetHandle, AssetLoadState::Unloaded, AssetLoadState::Queued, wasCreated);

		if (wasCreated)
		{
			// The asset was created by this thread, let's queue it for load.
			QueueAssetForLoading(assetHandle, newAsset, AssetLoadState::Queued);
		}

		AssetMetadata* metadata = m_assetRegistry.GetAssetMetadata(assetHandle);

		outAsset = newAsset.As<T>();
		return newAsset != nullptr && metadata->IsLoaded();
	}

	template<VoltAssetType T>
	bool AssetManager::TryGetAssetIfLoaded(AssetHandle assetHandle, AssetReference<T>& outAsset)
	{
		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(assetHandle);
		if (!assetMetadata.IsValid())
		{
			return false;
		}
		
		if (assetMetadata->IsLoaded())
		{
			outAsset = GetAssetImmediately<T>(assetHandle);
			return true;
		}

		return false;
	}

	template<VoltAssetType T, typename... Args>
	AssetReference<T> AssetManager::CreateMemoryAsset(StringView assetName, Args&&... args)
	{
		constexpr bool IsMemoryAsset = true;
		constexpr bool IsAnonymous = false;
		return CreateAssetImpl<T>(assetName, IsMemoryAsset, IsAnonymous, {}, std::forward<Args>(args)...);
	}

	template<VoltAssetType T, typename... Args> AssetReference<T>
	AssetManager::CreateAnonymousAsset(StringView assetName, Args&&... args)
	{
		constexpr bool IsMemoryAsset = true;
		constexpr bool IsAnonymous = true;
		return CreateAssetImpl<T>(assetName, IsMemoryAsset, IsAnonymous, {}, std::forward<Args>(args)...);
	}

	template<VoltAssetType T, typename... Args>
	AssetReference<T> AssetManager::CreateAsset(StringView assetName, Args&&... args)
	{
		constexpr bool IsMemoryAsset = false;
		constexpr bool IsAnonymous = false;
		return CreateAssetImpl<T>(assetName, IsMemoryAsset, IsAnonymous, {}, std::forward<Args>(args)...);
	}

	template<VoltAssetType T, typename... Args> AssetReference<T>
	AssetManager::CreateAssetWithAssetHandle(StringView assetName, AssetHandle assetHandle, Args&&... args)
	{
		constexpr bool IsMemoryAsset = false;
		constexpr bool IsAnonymous = false;
		return CreateAssetImpl<T>(assetName, IsMemoryAsset, IsAnonymous, assetHandle, std::forward<Args>(args)...);
	}

	template<VoltAssetType T, typename... Args>
	AssetReference<T> AssetManager::CreateAssetAndFile(const Filesystem::Path& targetDirectory, StringView assetName, Args&&... args)
	{
		AssetReference<T> asset = CreateAsset<T>(assetName, std::forward<Args>(args)...);

		Filesystem::Path targetPath = targetDirectory / (String(assetName) + ".vtasset");
		CreateFileForAsset(asset->GetAssetHandle(), targetPath);

		return asset;
	}

	template<VoltAssetType T, typename... Args> AssetReference<T>
	AssetManager::CreateAssetAndFileWithAssetHandle(const Filesystem::Path& targetDirectory, StringView assetName, AssetHandle assetHandle, Args&&... args)
	{
		AssetReference<T> asset = CreateAssetWithAssetHandle<T>(assetName, assetHandle, std::forward<Args>(args)...);

		Filesystem::Path targetPath = targetDirectory / (String(assetName) + ".vtasset");
		CreateFileForAsset(asset->GetAssetHandle(), targetPath);

		return asset;
	}

	template<VoltAssetType T, typename... Args>
	AssetReference<T> AssetManager::CreateAssetImpl(StringView assetName, bool isMemoryAsset, bool isAnonymous, AssetHandle assetHandle, Args&&... args)
	{
		IntRef<T> newAsset = m_assetAllocator.AllocateAsset<T>(std::forward<Args>(args)...);
		newAsset->AssignAssetHandle(assetHandle);

		AssetMetadata metadata{};
		metadata.filepath = ""; // Assets that are not saved will not have a file path
		metadata.handle = assetHandle;
		metadata.type = T::GetStaticType();
		metadata.m_loadState = AssetLoadState::Loaded;
		metadata.SetFlag(AssetMetadataFlag::MemoryOnly, isMemoryAsset);
		metadata.SetFlag(AssetMetadataFlag::Anonymous, isAnonymous);

		if (CustomAssetMetadataRegistry::Get().AssetTypeHasCustomMetadata(metadata.type))
		{
			CustomAssetMetadataRegistry::Get().SetupInitalCustomMetadata(metadata.type, metadata.customData);
		}

		newAsset->SetName(String(assetName));

		// Setup a link back to the asset manager.
		newAsset->m_referencedAssetManager = this;
		newAsset->m_generation = metadata.m_generation;

		newAsset->OnPreSave(metadata.customData);

		m_assetRegistry.InsertAssetMetadata(std::move(metadata));
		AddAssetToCache(newAsset),

		m_dependencyGraph->AddAssetToGraph(newAsset->GetAssetHandle());
		QueueAssetChanged(newAsset->GetAssetHandle(), AssetChangedState::Loaded);

		return newAsset;
	}
}
VTAS_API extern Unique<Volt::AssetManager> g_assetManager;
