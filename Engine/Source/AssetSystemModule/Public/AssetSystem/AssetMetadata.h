#pragma once

#include "AssetSystem/AssetType.h"
#include "AssetSystem/AssetHandle.h"

#include <CoreUtilities/Containers/Vector.h>

#include <shared_mutex>
#include <filesystem>

namespace Volt
{
	enum class AssetChangedState : uint8_t
	{
		Deleted,
		Loaded,
		Unloaded,
		Saved,
	};

	enum class AssetFlag : uint8_t
	{
		None = 0,
		Missing = BIT(0),
		Invalid = BIT(1),
		Queued = BIT(2),
		MemoryOnly = BIT(3)
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(AssetFlag);

	inline static constexpr size_t ASSET_CUSTOM_METADATA_SIZE = 256;
	typedef Vector<uint8_t, InlineAllocator<ASSET_CUSTOM_METADATA_SIZE>> CustomAssetMetadataVector;

	struct AssetMetadata
	{
		AssetMetadata() = default;
		AssetMetadata(const AssetMetadata& other)
		{
			handle = other.handle;
			type = other.type;
			isLoaded = other.isLoaded.load();
			isQueued = other.isQueued.load();
			isMemoryAsset = other.isMemoryAsset;
			filepath = other.filepath;
			customData = other.customData;
		}

		AssetMetadata& operator=(const AssetMetadata& other)
		{
			handle = other.handle;
			type = other.type;
			isLoaded = other.isLoaded.load();
			isQueued = other.isQueued.load();
			isMemoryAsset = other.isMemoryAsset;
			filepath = other.filepath;
			customData = other.customData;

			return *this;
		}

		VT_INLINE bool IsValid() const { return handle != 0; }
		VT_INLINE bool HasFilepath() const { return !filepath.empty(); }
		VT_INLINE bool IsMemoryAsset() const { return isMemoryAsset; }

		template<typename CustomMetadataType>
		VT_INLINE const CustomMetadataType& GetCustomData() const
		{
			VT_ENSURE_MSG(CustomMetadataType::IsForAssetType(type), std::format("Custom metadata type is not for type %s!", type->GetName()));
			VT_ENSURE_MSG(customData.size() == sizeof(CustomMetadataType), std::format("Custom metadata size is not correct, for type: %s!", type->GetName()));

			return reinterpret_cast<const CustomMetadataType&>(*customData.data());
		}

		AssetHandle handle = 0;
		AssetType type;

		std::atomic_bool isLoaded = false;
		std::atomic_bool isQueued = false;
		//a memory asset is an asset that is not saved to a file on the disk
		bool isMemoryAsset = false;

		std::filesystem::path filepath;

		CustomAssetMetadataVector customData;

	private:
		friend class WriteableAssetMetadata;
		friend class ReadOnlyAssetMetadata;
		friend class AssetManager_New;
		friend class AssetRegistry;

		std::shared_mutex m_assetMetadataMutex;
	};
}
