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

		AssetMetadata* GetAssetMetadata(AssetHandle assetHandle);
		AssetMetadata* GetAssetMetadata(AssetHandle assetHandle) const;

		VTAS_API void InsertAssetMetadata(AssetMetadata&& assetMetadata);

	private:
		void Initialize();
		
		void LoadAssetMetadata();
		void DeserializeAssetMetadata(const std::filesystem::path& filepath, AssetMetadata& outMetadata);

		// Returns a file path relative to either an engine asset directory,
		// or the project asset directory.
		std::filesystem::path GetRelativeAssetFilepath(const std::filesystem::path& filepath);

		// Returns all asset filepaths located within engine and project asset directories.
		Vector<std::filesystem::path> ScanForAssets();

		std::filesystem::path m_engineDirectoryPath;
		std::filesystem::path m_projectDirectoryPath;
		std::string_view m_assetsDirectoryName;

		AtomicHashTable<> m_hashTable;
		Vector<AssetMetadata*> m_metadataIndirection;
		PagedArenaAllocator<AssetMetadata, 2048> m_metadata;
	};
}
