#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetMetadataWrappers.h"
#include "AssetSystem/AssetDependencyGatherContext.h"

#include <CoreUtilities/Archive/Archive.h>

#include <atomic>

template<typename T>
class AssetReference;

namespace Volt
{
	class AssetRefCounter
	{
	public:
		AssetRefCounter(const AssetRefCounter&) noexcept = delete;
		AssetRefCounter& operator=(const AssetRefCounter&) noexcept = delete;
		AssetRefCounter(AssetRefCounter&&) noexcept = delete;
		AssetRefCounter& operator=(AssetRefCounter&&) noexcept = delete;
	
		VT_INLINE void IncRef() const noexcept
		{
			[[maybe_unused]] auto oldValue = m_refCount.fetch_add(1u, std::memory_order::relaxed);
			VT_ASSERT(oldValue > 0);
		}

		void DecRef() const noexcept
		{
			auto oldCount = m_refCount.fetch_sub(1, std::memory_order::release);
			VT_ASSERT(oldCount > 0);

			// The last reference is held in the asset cache.
			if (oldCount == 2)
			{
				std::atomic_thread_fence(std::memory_order::acquire);
				Evict();
			}
		}

		VT_INLINE int32_t GetRefCount() const noexcept
		{
			return m_refCount.load(std::memory_order::relaxed);
		}

	protected:
		AssetRefCounter() noexcept = default;
		virtual ~AssetRefCounter()
		{
			[[maybe_unused]] auto validCount = [](auto val) { return val == 0 || val == 1; };
			VT_ASSERT(validCount(m_refCount.load(std::memory_order::relaxed)));
		}

	private:
		friend class AssetManager;
		friend class AssetCache;

		VTAS_API void Evict() const;

		mutable class AssetManager* m_referencedAssetManager = nullptr;
		mutable std::atomic<int32_t> m_refCount = 1;
	};

	class Asset : public AssetRefCounter
	{
	public:
		Asset(const Asset&) noexcept = delete;
		Asset& operator=(const Asset&) noexcept = delete;
		Asset(Asset&&) noexcept = delete;
		Asset& operator=(Asset&&) noexcept = delete;

		~Asset() override = default;

		virtual AssetType GetType() const { return AssetTypes::None; }
		virtual uint32_t GetVersion() const { return 1; }

		/*
			Called when a dependency of this asset has changed state (loaded, unloaded, etc).
		*/
		virtual void OnAssetDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state) {}

		/*
			Called when the name of the asset is changed.
		*/
		virtual void OnAssetNameChanged() {}

		/*
			Allows the asset to fill the custom asset metadata before it is saved.
			Or perform other pre save actions.
		*/
		virtual void OnPreSave(CustomAssetMetadata& customMetadata) {}

		/*
			Called when the asset is saved/loaded.
		*/
		virtual void Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata) {}

		/*
			Called when asset dependencies are gathered.
			Assets should define all of it's dependencies here.
		*/
		virtual void GatherAssetDependencies(AssetDependencyGatherContext& gatherContext, ReadOnlyAssetMetadata assetMetadata) {}

		VT_NODISCARD VT_INLINE const AssetHandle& GetAssetHandle() const { return m_handle; }
		VT_NODISCARD VT_INLINE StringView GetAssetName() const { return m_name; }
		VT_NODISCARD VT_INLINE bool IsFlagSet(AssetFlag flag) const;

		VT_INLINE void SetName(const String& name);
		VT_INLINE void SetFlag(AssetFlag flag, bool state);

		VT_INLINE bool operator==(const Asset& other) { return m_handle == other.m_handle; }
		VT_INLINE bool operator!=(const Asset& other) { return m_handle != other.m_handle; }

		VT_NODISCARD VT_INLINE static const AssetHandle Null() { return AssetHandle(0); }
		VT_NODISCARD VT_INLINE bool IsValid() const { return (!IsFlagSet(AssetFlag::Invalid) && !IsFlagSet(AssetFlag::Missing)); }

	protected:
		Asset() noexcept = default;

	private:
		friend class AssetAllocator;
		friend class AssetManager;
		friend class AssetCache;

		VT_INLINE void AssignAssetHandle(AssetHandle assetHandle);

		String m_name;
		AssetHandle m_handle = Null();
		uint64_t m_generation;
		std::atomic_uint8_t m_assetFlags = static_cast<uint8_t>(AssetFlag::None);
	};

	VT_NODISCARD VT_INLINE bool Asset::IsFlagSet(AssetFlag flag) const
	{
		AssetFlag value = static_cast<AssetFlag>(m_assetFlags.load(std::memory_order::relaxed) & static_cast<uint8_t>(flag));
		return value != AssetFlag::None;
	}

	VT_INLINE void Asset::SetName(const String& name)
	{
		if (name == m_name)
		{
			return;
		}

		m_name = name;
		OnAssetNameChanged();
	}

	VT_INLINE void Asset::SetFlag(AssetFlag flag, bool state)
	{
		if (state)
		{
			m_assetFlags.fetch_or(static_cast<uint8_t>(flag), std::memory_order::relaxed);
		}
		else
		{
			m_assetFlags.fetch_and(static_cast<uint8_t>(~flag), std::memory_order::relaxed);
		}
	}

	VT_INLINE void Asset::AssignAssetHandle(AssetHandle assetHandle)
	{
		m_handle = assetHandle;
	}
}
