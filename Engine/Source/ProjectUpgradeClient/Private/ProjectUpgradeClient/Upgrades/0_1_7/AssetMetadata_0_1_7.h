#pragma once

#include <AssetSystem/Asset.h>

namespace Volt
{
	static constexpr size_t ASSET_CUSTOM_METADATA_SIZE = 256;
	typedef Vector<uint8_t, InlineAllocator<ASSET_CUSTOM_METADATA_SIZE>> CustomAssetMetadataVector;

	struct AssetMetadata_0_1_7
	{
		AssetMetadata_0_1_7() = default;
		AssetMetadata_0_1_7(const AssetMetadata_0_1_7& other)
		{
			handle = other.handle;
			type = other.type;
			flags = other.flags.load();
			filepath = other.filepath;
			customData = other.customData;
		}

		AssetMetadata_0_1_7& operator=(const AssetMetadata_0_1_7& other)
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

		VT_INLINE bool IsValid() const { return handle != 0 && !IsFlagSet(AssetMetadataFlag::Removed); }
		VT_INLINE bool HasFilepath() const { return !filepath.empty(); }
		VT_INLINE bool IsMemoryAsset() const { return IsFlagSet(AssetMetadataFlag::MemoryOnly); }

		template<typename CustomMetadataType>
		VT_INLINE const CustomMetadataType& GetCustomData() const
		{
			VT_ENSURE_MSG(CustomMetadataType::IsForAssetType(type), std::format("Custom metadata type is not for type {}!", type->GetName()));
			VT_ENSURE_MSG(customData.size() == sizeof(CustomMetadataType), std::format("Custom metadata size is not correct, for type: {}!", type->GetName()));

			return reinterpret_cast<const CustomMetadataType&>(*customData.data());
		}

		VT_INLINE friend Archive& operator<<(Archive& archive, AssetMetadata_0_1_7& value)
		{
			archive << value.handle;
			VoltGUID assetTypeGUID;

			if (!archive.IsLoading())
			{
				assetTypeGUID = value.type->GetGUID();
			}

			archive << assetTypeGUID;

			if (archive.IsLoading())
			{
				value.type = GetAssetTypeRegistry().GetTypeFromGUID(assetTypeGUID);
			}

			archive << value.customData;
			return archive;
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

	bool AssetMetadata_0_1_7::IsFlagSet(AssetMetadataFlag flag) const
	{
		AssetFlag value = static_cast<AssetFlag>(flags.load(std::memory_order::relaxed) & static_cast<uint8_t>(flag));
		return value != AssetFlag::None;
	}

	void AssetMetadata_0_1_7::SetFlag(AssetMetadataFlag flag, bool state)
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
