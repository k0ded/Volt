#pragma once

#include "AssetSystem/Asset.h"

#include <CoreUtilities/Containers/AtomicHashTable.h>
#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

#include <JobSystem/Job.h>

#include <set>

namespace Volt
{
	class AssetRegistry
	{
	public:
		enum class AssetHeaderDeserializationResult
		{
			InvalidAssetFile,
			InvalidVersion,
			Success
		};

		AssetRegistry(const std::filesystem::path& engineDirectoryPath, const std::filesystem::path& projectDirectoryPath, std::string_view assetsDirectoryName);
		~AssetRegistry();

		VTAS_API AssetMetadata* GetAssetMetadata(AssetHandle assetHandle);
		VTAS_API AssetMetadata* GetAssetMetadata(AssetHandle assetHandle) const;

		VTAS_API bool IsValidAssetHandle(AssetHandle assetHandle) const;

		VTAS_API void InsertAssetMetadata(AssetMetadata&& assetMetadata);
		void RemoveAssetMetadata(AssetHandle assetHandle, bool unlockMutex = false);

		// Returns a file path relative to either an engine asset directory,
		// or the project asset directory.
		std::filesystem::path GetRelativeAssetFilepath(const std::filesystem::path& filepath) const;

		JobCounterRef GetMetadataLoadingCounter() { return m_metadataLoadingCounter; }

		VTAS_API static int32_t GetNumMaxAssets();
		static AssetHeaderDeserializationResult DeserializeAssetHeader(Archive& archive, AssetMetadata& outAssetMetadata, uint32_t expectedAssetVersion, bool checkAssetVersion);

	private:
		friend class AssetRegistryIterator;
		friend class AssetRegistryConstIterator;

		using AssetMetadataAllocator = PagedAtomicArenaAllocator<AssetMetadata, 2048>;

		void Initialize();
		
		void LoadAssetMetadata();
		void DeserializeAssetMetadata(const std::filesystem::path& filepath, AssetMetadata& outMetadata);

		// Returns all asset filepaths located within engine and project asset directories.
		void ScanForAssets(Vector<std::filesystem::path>& outEngineAssets, Vector<std::filesystem::path>& outProjectAssets);

		std::filesystem::path m_engineDirectoryPath;
		std::filesystem::path m_projectDirectoryPath;
		std::string_view m_assetsDirectoryName;

		AtomicHashTable<> m_hashTable;
		Vector<AssetMetadata*> m_metadataIndirection;
		AssetMetadataAllocator m_metadata;
		JobCounterRef m_metadataLoadingCounter;
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
		// Whether or not to include memory assets.
		bool includeMemoryAssets = true;

		// Whether or not to include non memory assets without filepaths
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
