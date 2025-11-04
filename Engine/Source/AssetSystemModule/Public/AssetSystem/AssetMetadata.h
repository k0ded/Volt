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

	enum class AssetMetadataFlag : uint8_t
	{
		None = 0,
		Loaded = BIT(0),
		Queued = BIT(1),
		MemoryOnly = BIT(2),
		Anonymous = BIT(3)
	};
	VT_SETUP_ENUM_CLASS_OPERATORS(AssetMetadataFlag);

	inline static constexpr size_t ASSET_CUSTOM_METADATA_SIZE = 256;
	typedef Vector<uint8_t, InlineAllocator<ASSET_CUSTOM_METADATA_SIZE>> CustomAssetMetadataVector;

	struct AssetMetadata
	{
		AssetMetadata() = default;
		AssetMetadata(const AssetMetadata& other)
		{
			handle = other.handle;
			type = other.type;
			flags = other.flags.load();
			filepath = other.filepath;
			customData = other.customData;
		}

		AssetMetadata& operator=(const AssetMetadata& other)
		{
			handle = other.handle;
			type = other.type;
			flags = other.flags.load();
			filepath = other.filepath;
			customData = other.customData;

			return *this;
		}

		VT_NODISCARD VT_INLINE bool IsFlagSet(AssetMetadataFlag flag) const;
		VT_INLINE void SetFlag(AssetMetadataFlag flag, bool state);

		VT_INLINE bool IsValid() const { return handle != 0; }
		VT_INLINE bool HasFilepath() const { return !filepath.empty(); }
		VT_INLINE bool IsMemoryAsset() const { return IsFlagSet(AssetMetadataFlag::MemoryOnly); }
		VT_INLINE bool IsLoaded() const { return IsFlagSet(AssetMetadataFlag::Loaded); }

		template<typename CustomMetadataType>
		VT_INLINE const CustomMetadataType& GetCustomData() const
		{
			VT_ENSURE_MSG(CustomMetadataType::IsForAssetType(type), std::format("Custom metadata type is not for type %s!", type->GetName()));
			VT_ENSURE_MSG(customData.size() == sizeof(CustomMetadataType), std::format("Custom metadata size is not correct, for type: %s!", type->GetName()));

			return reinterpret_cast<const CustomMetadataType&>(*customData.data());
		}

		AssetHandle handle = 0;
		AssetType type;

		std::atomic_uint8_t flags = static_cast<uint8_t>(AssetMetadataFlag::None);
		std::filesystem::path filepath;

		CustomAssetMetadataVector customData;

	private:
		friend class WriteableAssetMetadata;
		friend class ReadOnlyAssetMetadata;
		friend class AssetManager;
		friend class AssetRegistry;

		std::shared_mutex m_assetMetadataMutex;
	};

	bool AssetMetadata::IsFlagSet(AssetMetadataFlag flag) const
	{
		AssetFlag value = static_cast<AssetFlag>(flags.load(std::memory_order::relaxed) & static_cast<uint8_t>(flag));
		return value != AssetFlag::None;
	}

	void AssetMetadata::SetFlag(AssetMetadataFlag flag, bool state)
	{
		if (state)
		{
			flags.fetch_or(static_cast<uint8_t>(flag), std::memory_order::relaxed);
		}
		else
		{
			flags.fetch_and(static_cast<uint8_t>(~flag), std::memory_order::relaxed);
		}
	}
}
