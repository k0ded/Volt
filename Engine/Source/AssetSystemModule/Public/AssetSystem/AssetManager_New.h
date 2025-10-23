#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetRegistry.h"
#include "AssetSystem/AssetAllocator.h"
#include "AssetSystem/AssetCache.h"
#include "AssetSystem/AssetReference.h"
#include "AssetSystem/AssetSerializerRegistry.h"

#include <CoreUtilities/Pointers/RefPtr.h>

#include <filesystem>

namespace Volt
{
	// A wrapper of the asset metadata that ensures no other
	// thread can access it at the same time.
	// Takes a asset metadata as a reference, it is ok as all metadata
	// has stable pointers.
	class WriteableAssetMetadata
	{
	public:
		WriteableAssetMetadata(AssetMetadata* inMetadata) noexcept
			: m_metadata(inMetadata)
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.lock();
			}
		}

		~WriteableAssetMetadata() noexcept
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.unlock();
			}
		}

		WriteableAssetMetadata(WriteableAssetMetadata&&) = delete;
		WriteableAssetMetadata(const WriteableAssetMetadata&) = delete;
		WriteableAssetMetadata& operator=(WriteableAssetMetadata&&) = delete;
		WriteableAssetMetadata& operator=(const WriteableAssetMetadata&) = delete;

		VT_INLINE AssetMetadata* operator->() noexcept
		{
			return m_metadata;
		}

		VT_INLINE AssetMetadata& operator*() noexcept
		{
			return *m_metadata;
		}

		VT_INLINE bool IsValid() const 
		{
			return m_metadata != nullptr;
		}

	private:
		AssetMetadata* m_metadata;
	};

	class ReadOnlyAssetMetadata
	{
	public:
		ReadOnlyAssetMetadata(AssetMetadata* inMetadata) noexcept
			: m_metadata(inMetadata)
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.lock_shared();
			}
		}

		~ReadOnlyAssetMetadata() noexcept
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.unlock_shared();
			}
		}

		VT_INLINE ReadOnlyAssetMetadata(ReadOnlyAssetMetadata&& other) noexcept
		{
			// We take control of the lock from the moved asset.
			m_metadata = other.m_metadata;
			other.m_metadata = nullptr;
		}

		VT_INLINE ReadOnlyAssetMetadata(const ReadOnlyAssetMetadata& other) noexcept
		{
			// Lock the asset meta for this instance.
			m_metadata = other.m_metadata;
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.lock_shared();
			}
		}

		VT_INLINE ReadOnlyAssetMetadata& operator=(ReadOnlyAssetMetadata&& other) noexcept
		{
			// We take control of the lock from the moved asset.
			m_metadata = other.m_metadata;
			other.m_metadata = nullptr;

			return *this;
		}

		VT_INLINE ReadOnlyAssetMetadata& operator=(const ReadOnlyAssetMetadata& other) noexcept
		{
			// Lock the asset meta for this instance.
			m_metadata = other.m_metadata;
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.lock_shared();
			}

			return *this;
		}

		VT_INLINE const AssetMetadata* operator->() const noexcept
		{
			return m_metadata;
		}

		VT_INLINE const AssetMetadata& operator*() const noexcept
		{
			return *m_metadata;
		}

		VT_INLINE bool IsValid() const
		{
			return m_metadata != nullptr;
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
		WriteableAssetMetadata GetWriteableAssetMetadata(AssetHandle assetHandle) const;
		ReadOnlyAssetMetadata GetReadOnlyAssetMetadata(AssetHandle assetHandle) const;
		AssetMetadata GetAssetMetadataCopy(AssetHandle assetHandle) const;

		///// Asset /////
		// Will unload and load the asset again, the pointer to the asset will remain the same,
		// to allow other systems to keep a reference to the asset.
		void ReloadAsset(AssetHandle assetHandle);

		// Will trigger a serialization of the asset, if it has an assigned filepath.
		void SaveAsset(AssetHandle assetHandle);
		void SaveAsset(AssetReference<Asset_New> asset);

		// Returns wether or not the asset exists in the asset registry.
		bool IsValidAssetHandle(AssetHandle assetHandle) const;

		// Will return the requested asset if loaded, will otherwise stall until the asset has been loaded.
		template<VoltAssetType T> AssetReference<T> GetAssetImmediately(AssetHandle assetHandle);
		// Will return true and the asset if it is loaded, if the asset is not loaded it will queue it for loading.
		template<VoltAssetType T> bool TryGetAsset(AssetHandle assetHandle, AssetReference<T>& outAsset);

		// Creates an asset that only lives in memory during the current application run, is not serializable to disk.
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateMemoryAsset(std::string_view assetName, Args&&... args);
		// Creates an asset that does not have a filepath yet.
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAsset(std::string_view assetName, Args&&... args);
		// Creates an asset, assigns a filepath and creates the asset disk file itself.
		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAssetAndFile(const std::filesystem::path& targetDirectory, std::string_view assetName, Args&&... args);

		void CreateFileForAsset(AssetHandle assetHandle, const std::filesystem::path& filepath);

		///// Management /////
		AssetUpdatedCallbackID RegisterAssetUpdatedCallback(AssetType assetType, AssetChangedCallback&& callback);
		void UnregisterAssetUpdatedCallback(AssetType assetType, UUID64 callbackId);

		void AddDependencyToAsset(AssetHandle dependant, AssetHandle dependency);
		Vector<AssetHandle> GetAssetsDependentOn(AssetHandle assetHandle) const;

	private:
		friend class AssetRefCounter;

		struct ScopedAssetLock
		{
			ScopedAssetLock(RefPtr<Asset_New> asset);
			~ScopedAssetLock();

		private:
			RefPtr<Asset_New> m_asset;
		};

		template<VoltAssetType T, typename... Args> AssetReference<T> CreateAssetImpl(std::string_view assetName, bool isMemoryAsset, Args&&... args);

		void LoadAsset(AssetHandle assetHandle, RefPtr<Asset_New> asset);
		void UnloadAndFreeAsset(AssetRefCounter* assetRefCounter);

		AssetRegistry m_assetRegistry;
		AssetAllocator m_assetAllocator;
		AssetCache m_assetCache;
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

		{
			ReadOnlyAssetMetadata metadata = m_assetRegistry.GetAssetMetadata(assetHandle);

			// All metadatas should be valid.
			VT_ENSURE(metadata->IsValid());

			newAsset->AssignAssetHandle(metadata->handle);
			newAsset->SetName(metadata->filePath.stem().string());
		}

		LoadAsset(assetHandle, newAsset);
	}

	template<VoltAssetType T>
	bool AssetManager_New::TryGetAsset(AssetHandle assetHandle, AssetReference<T>& outAsset)
	{

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

		std::filesystem::path targetPath = targetDirectory / (assetName + ".vtasset");
		CreateFileForAsset(asset->GetAssetHandle(), targetPath);
	}

	template<VoltAssetType T, typename... Args> 
	AssetReference<T> AssetManager_New::CreateAssetImpl(std::string_view assetName, bool isMemoryAsset, Args&&... args)
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
		newAsset->m_referencedAssetManager = this;

		m_assetRegistry.InsertAssetMetadata(std::move(metadata));

		m_assetCache.AddAsset(newAsset);

		return newAsset;
	}

	VTAS_API extern Scope<AssetManager_New> g_assetManager;
}
