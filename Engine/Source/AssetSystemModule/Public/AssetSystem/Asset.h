#pragma once

#include "AssetSystem/AssetType.h"
#include "AssetSystem/AssetHandle.h"

#include <CoreUtilities/Containers/Vector.h>

#include <filesystem>
#include <shared_mutex>

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
			isLoaded = other.isLoaded;
			isQueued = other.isQueued;
			isMemoryAsset = other.isMemoryAsset;
			filepath = other.filepath;
			customData = other.customData;
		}

		AssetMetadata& operator=(const AssetMetadata& other)
		{
			handle = other.handle;
			type = other.type;
			isLoaded = other.isLoaded;
			isQueued = other.isQueued;
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

		bool isLoaded = false;
		bool isQueued = false;
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

	// #TODO_Ivar: Change name to be getter / setter, also add virtual functions when name changes.
	class Asset
	{
	public:
		virtual ~Asset() = default;

		VT_NODISCARD VT_INLINE bool IsValid() const { return ((assetFlags & AssetFlag::Missing) | (assetFlags & AssetFlag::Invalid) | (assetFlags & AssetFlag::Queued)) == AssetFlag::None; }

		inline virtual bool operator==(const Asset& other)
		{
			return handle = other.handle;
		}

		inline virtual bool operator!=(const Asset& other)
		{
			return !(*this == other);
		}

		VT_NODISCARD VT_INLINE bool IsFlagSet(AssetFlag flag) { return (assetFlags & flag) != AssetFlag::None; }
		VT_INLINE void SetFlag(AssetFlag flag, bool state)
		{
			if (state)
			{
				assetFlags |= flag;
			}
			else
			{
				assetFlags &= ~flag;
			}
		}

		VT_NODISCARD VT_INLINE static const AssetHandle Null() { return AssetHandle(0); }

		virtual AssetType GetType() { return AssetTypes::None; }
		virtual uint32_t GetVersion() const { return 1; }
		virtual void OnDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state) {}
		virtual void SetupInitialCustomMetadata(CustomAssetMetadataVector& customMetadata) {}

		AssetFlag assetFlags = AssetFlag::None;
		AssetHandle handle = {};
		std::string assetName;
	};
}
