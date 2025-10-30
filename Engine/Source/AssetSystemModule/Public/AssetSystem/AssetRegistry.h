#pragma once

#include "AssetSystem/Asset_New.h"

#include <CoreUtilities/Containers/AtomicHashTable.h>
#include <CoreUtilities/Allocators/PagedArenaAllocator.h>

namespace Volt
{
	class AssetRegistry
	{
	public:
		AssetRegistry(const std::filesystem::path& engineDirectoryPath, const std::filesystem::path& projectDirectoryPath, std::string_view assetsDirectoryName);

		VTAS_API AssetMetadata* GetAssetMetadata(AssetHandle assetHandle);
		VTAS_API AssetMetadata* GetAssetMetadata(AssetHandle assetHandle) const;

		VTAS_API bool IsValidAssetHandle(AssetHandle assetHandle) const;

		VTAS_API void InsertAssetMetadata(AssetMetadata&& assetMetadata);
		void RemoveAssetMetadata(AssetHandle assetHandle, bool unlockMutex = false);

		// Returns a file path relative to either an engine asset directory,
		// or the project asset directory.
		std::filesystem::path GetRelativeAssetFilepath(const std::filesystem::path& filepath) const;

	private:
		friend class AssetRegistryIterator;
		friend class AssetRegistryConstIterator;

		using AssetMetadataAllocator = PagedArenaAllocator<AssetMetadata, 2048>;

		void Initialize();
		
		void LoadAssetMetadata();
		void DeserializeAssetMetadata(const std::filesystem::path& filepath, AssetMetadata& outMetadata);

		// Returns all asset filepaths located within engine and project asset directories.
		// #TODO_AssetSystem: Change to take a vector reference instead.
		Vector<std::filesystem::path> ScanForAssets();

		std::filesystem::path m_engineDirectoryPath;
		std::filesystem::path m_projectDirectoryPath;
		std::string_view m_assetsDirectoryName;

		AtomicHashTable<> m_hashTable;
		Vector<AssetMetadata*> m_metadataIndirection;
		AssetMetadataAllocator m_metadata;
	};

	enum class AssetMetadataInit
	{
		Null
	};

	// A wrapper of the asset metadata that ensures no other
	// thread can access it at the same time.
	// Takes a asset metadata as a reference, it is ok as all metadata
	// has stable pointers.
	class WriteableAssetMetadata
	{
	public:
		WriteableAssetMetadata(AssetMetadataInit) noexcept
			: m_metadata(nullptr)
		{}

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
		ReadOnlyAssetMetadata(AssetMetadataInit) noexcept
			: m_metadata(nullptr)
		{}

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

	class AssetRegistryIterator
	{
	public:
		AssetRegistryIterator(AssetRegistry& assetRegistry)
			: m_assetRegistry(assetRegistry),
			m_iterator(assetRegistry.m_metadata)
		{}
	
		VT_INLINE void operator++()
		{
			++m_iterator;
		}

		VT_INLINE WriteableAssetMetadata operator*() const
		{
			return *m_iterator;
		}

		VT_INLINE explicit operator bool() const
		{
			return bool(m_iterator);
		}

	private:
		AssetRegistry::AssetMetadataAllocator::Iterator m_iterator;
		AssetRegistry& m_assetRegistry;
	};

	class AssetRegistryConstIterator
	{
	public:
		AssetRegistryConstIterator(const AssetRegistry& assetRegistry)
			: m_assetRegistry(assetRegistry),
			m_iterator(assetRegistry.m_metadata)
		{}

		VT_INLINE void operator++()
		{
			++m_iterator;
		}

		VT_INLINE ReadOnlyAssetMetadata operator*() const
		{
			return *m_iterator;
		}

		VT_INLINE explicit operator bool() const
		{
			return bool(m_iterator);
		}

	private:
		AssetRegistry::AssetMetadataAllocator::Iterator m_iterator;
		const AssetRegistry& m_assetRegistry;
	};

	struct AssetRegistryIteratorFilter
	{
		// Wether or not to include memory assets.
		bool includeMemoryAssets = true;

		// Wether or not to include non memory assets without filepaths
		bool includeWithoutFilepath = true;

		// If empty, all types are considered. Otherwise only the ones in this list.
		std::set<AssetType> filteredAssetTypes;

		template<typename T>
		void AddAssetType()
		{
			filteredAssetTypes.insert(T::GetStaticType());
		}
	};
}
