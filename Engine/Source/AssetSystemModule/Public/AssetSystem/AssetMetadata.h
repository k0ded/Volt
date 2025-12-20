#pragma once

#include "AssetSystem/AssetType.h"
#include "AssetSystem/AssetHandle.h"
#include "AssetSystem/CustomAssetMetadataRegistry.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Any.h>

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
		MemoryOnly = BIT(3)
	};
	VT_SETUP_ENUM_CLASS_OPERATORS(AssetFlag);

	enum class AssetMetadataFlag : uint8_t
	{
		None = 0,
		Missing = BIT(0),
		Invalid = BIT(1),
		MemoryOnly = BIT(2),
		Anonymous = BIT(3)
	};
	VT_SETUP_ENUM_CLASS_OPERATORS(AssetMetadataFlag);

	enum class AssetLoadState : uint8_t
	{
		Unloaded,
		Queued,
		Loading,
		Loaded,
		Unloading
	};

	struct AssetMetadataArchiveVersion
	{
		enum Type
		{
			BaseVersion = 0,

			// Switched to dynamic storage for custom asset metadata
			NewCustomMetadataStorage = 1,

			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		inline static constexpr VoltGUID guid = "{24A3A5FF-8FBE-4FBC-BF7E-362AD00D0183}"_guid;

	private:
		AssetMetadataArchiveVersion() {}
	};

	struct CustomAssetMetadataArchiveVersion
	{
		enum Type
		{
			BaseVersion = 0,

			AddedCheckIfDataIsSaved = 1,

			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		inline static constexpr VoltGUID guid = "{8BAC14B2-A3E8-4872-A5A3-C036FD2CE383}"_guid;

	private:
		CustomAssetMetadataArchiveVersion() {}
	};

	class CustomAssetMetadata
	{
	public:
		CustomAssetMetadata(const AssetType& assetType)
			: m_assetType(assetType)
		{}

		CustomAssetMetadata(const CustomAssetMetadata& other)
			: m_assetType(other.m_assetType),
			m_storage(other.m_storage)
		{}

		CustomAssetMetadata(CustomAssetMetadata&& other)
			: m_assetType(other.m_assetType),
			m_storage(std::move(other.m_storage))
		{}

		CustomAssetMetadata& operator=(const CustomAssetMetadata& other)
		{
			if (this != &other)
			{
				m_storage = other.m_storage;
			}

			return *this;
		}

		CustomAssetMetadata& operator=(CustomAssetMetadata&& other)
		{
			if (this != &other)
			{
				m_storage = std::move(other.m_storage);
			}

			return *this;
		}

		template<typename CustomMetadataType>
		VT_INLINE const CustomMetadataType& GetCustomMetadata() const
		{
			VT_ENSURE_MSG(CustomMetadataType::IsForAssetType(m_assetType), std::format("Custom metadata type is not for type {}!", m_assetType->GetName()));

			return m_storage.Cast<CustomMetadataType>();
		}

		template<typename CustomMetadataType>
		VT_INLINE CustomMetadataType& GetMutableCustomMetadata()
		{
			VT_ENSURE_MSG(CustomMetadataType::IsForAssetType(m_assetType), std::format("Custom metadata type is not for type {}!", m_assetType->GetName()));

			return m_storage.Cast<CustomMetadataType>();
		}

		template<typename CustomMetadataType>
		VT_INLINE void InitializeWithType()
		{
			m_storage.Emplace<CustomMetadataType>(CustomMetadataType());
		}

		VT_INLINE Any& GetStorage()
		{
			return m_storage;
		}

		VT_INLINE friend Archive& operator<<(Archive& archive, CustomAssetMetadata& value)
		{
			archive.UseVersion(CustomAssetMetadataArchiveVersion::guid);

			bool hasData = value.m_storage.HasValue();

			if (!archive.IsLoading())
			{
				archive << hasData;
			}
			else
			{
				if (archive.GetVersion(CustomAssetMetadataArchiveVersion::guid) >= CustomAssetMetadataArchiveVersion::AddedCheckIfDataIsSaved)
				{
					archive << hasData;
				}
			}

			if (hasData)
			{
				if (CustomAssetMetadataRegistry::Get().AssetTypeHasCustomMetadata(value.m_assetType))
				{
					CustomAssetMetadataRegistry::Get().SerializeAny(value.m_assetType, value.m_storage, archive);
				}
			}

			return archive;
		}

	private:
		Any m_storage;
		const AssetType& m_assetType;
	};

	struct AssetMetadata
	{
		AssetMetadata()
			: customData(type)
		{}

		AssetMetadata(const AssetMetadata& other)
			: customData(other.customData)
		{
			handle = other.handle;
			type = other.type;
			flags = other.flags.load();
			filepath = other.filepath;
			customData = other.customData;
			m_loadState = other.m_loadState.load();
			m_generation = other.m_generation.load();
		}

		AssetMetadata& operator=(const AssetMetadata& other)
		{
			handle = other.handle;
			type = other.type;
			flags = other.flags.load();
			filepath = other.filepath;
			customData = other.customData;
			m_loadState = other.m_loadState.load();
			m_generation = other.m_generation.load();

			return *this;
		}

		VT_NODISCARD VT_INLINE bool IsFlagSet(AssetMetadataFlag flag) const;
		VT_INLINE void SetFlag(AssetMetadataFlag flag, bool state);

		VT_INLINE bool IsValid() const { return handle != 0; }
		VT_INLINE bool HasFilepath() const { return !filepath.empty(); }
		VT_INLINE bool IsMemoryAsset() const { return IsFlagSet(AssetMetadataFlag::MemoryOnly); }
		VT_INLINE bool IsLoaded() const { return m_loadState.load(std::memory_order::relaxed) == AssetLoadState::Loaded; }

		template<typename CustomMetadataType>
		VT_INLINE const CustomMetadataType& GetCustomData() const
		{
			VT_ENSURE_MSG(CustomMetadataType::IsForAssetType(type), std::format("Custom metadata type is not for type {}!", type->GetName()));

			return customData.GetCustomMetadata<CustomMetadataType>();
		}

		VT_INLINE friend Archive& operator<<(Archive& archive, AssetMetadata& value)
		{
			archive.UseVersion(AssetMetadataArchiveVersion::guid);

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

			if (archive.IsLoading() && archive.GetVersion(AssetMetadataArchiveVersion::guid) < AssetMetadataArchiveVersion::NewCustomMetadataStorage)
			{
				constexpr size_t ASSET_CUSTOM_METADATA_SIZE = 256;
				typedef Vector<uint8_t, InlineAllocator<ASSET_CUSTOM_METADATA_SIZE>> CustomAssetMetadataVector;
				CustomAssetMetadataVector tempVector;
				archive << tempVector;
			}
			else
			{
				archive << value.customData;
			}

			return archive;
		}

		AssetHandle handle = 0;
		AssetType type;

		std::atomic_uint8_t flags = static_cast<uint8_t>(AssetMetadataFlag::None);
		std::filesystem::path filepath;

		CustomAssetMetadata customData;

	private:
		friend class WriteableAssetMetadata;
		friend class ReadOnlyAssetMetadata;
		friend class AssetManager;
		friend class AssetRegistry;

		VT_INLINE bool TryTransitionLoadState(AssetLoadState& expectedLoadState, AssetLoadState desiredLoadState)
		{
			const bool succeeded = m_loadState.compare_exchange_strong(expectedLoadState, desiredLoadState, std::memory_order::acq_rel);
			return succeeded;
		}

		VT_INLINE uint64_t GetGeneration(std::memory_order memoryOrder = std::memory_order::relaxed)
		{
			return m_generation.load(memoryOrder);
		}

		std::atomic<AssetLoadState> m_loadState = AssetLoadState::Unloaded;
		std::atomic<uint64_t> m_generation = 1;
		std::atomic<uint64_t> m_publishedGeneration = 0;

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
