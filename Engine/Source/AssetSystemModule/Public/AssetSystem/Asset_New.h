#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetMetadata.h"

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

		VT_INLINE void DecRef() const noexcept
		{
			auto oldCount = m_refCount.fetch_sub(1, std::memory_order::release);
			VT_ASSERT(oldCount > 0);

			// In the case of assets, the asset manager should always keep a reference,
			// meaning that when there is one reference left, the asset should be unloaded and destroyed.
			if (oldCount == 2)
			{
				std::atomic_thread_fence(std::memory_order::acquire);
				Unload();
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
		friend class AssetManager_New;

		VTAS_API void Unload() const;

		mutable class AssetManager_New* m_referencedAssetManager = nullptr;
		mutable std::atomic<int32_t> m_refCount = 1;
	};

	class AssetLocks
	{
	public:
		virtual ~AssetLocks() = default;

	private:
		template<typename T>
		friend class AssetReference;

		friend class AssetManager_New;

		mutable std::shared_mutex m_assetMutex;
	};

	class Asset_New : public AssetRefCounter, public AssetLocks
	{
	public:
		Asset_New(const Asset_New&) noexcept = delete;
		Asset_New& operator=(const Asset_New&) noexcept = delete;
		Asset_New(Asset_New&&) noexcept = delete;
		Asset_New& operator=(Asset_New&&) noexcept = delete;

		~Asset_New() override = default;

		virtual AssetType GetType() const { return AssetTypes::None; }
		virtual uint32_t GetVersion() const { return 1; }
		virtual void OnAssetDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state) {}
		virtual void OnAssetNameChanged() {}
		virtual void SetupInitialCustomMetadata(CustomAssetMetadataVector& customMetadata) {}

		VT_NODISCARD VT_INLINE const AssetHandle& GetAssetHandle() const { return m_handle; }
		VT_NODISCARD VT_INLINE std::string_view GetAssetName() const { return m_name; }
		VT_NODISCARD VT_INLINE bool IsFlagSet(AssetFlag flag) const;

		VT_INLINE void SetName(const std::string& name);
		VT_INLINE void SetFlag(AssetFlag flag, bool state);

		VT_INLINE bool operator==(const Asset_New& other) { return m_handle == other.m_handle; }
		VT_INLINE bool operator!=(const Asset_New& other) { return m_handle != other.m_handle; }

		VT_NODISCARD VT_INLINE static const AssetHandle Null() { return AssetHandle(0); }
		VT_NODISCARD VT_INLINE bool IsValid() const { return (!IsFlagSet(AssetFlag::Invalid) && !IsFlagSet(AssetFlag::Missing) && !IsFlagSet(AssetFlag::Queued)); }

	protected:
		Asset_New() noexcept = default;

	private:
		friend class AssetAllocator;
		friend class AssetManager_New;

		VT_INLINE void AssignAssetHandle(AssetHandle assetHandle);

		std::string m_name;
		AssetHandle m_handle = Null();
		std::atomic_uint8_t m_assetFlags = static_cast<uint8_t>(AssetFlag::None);
	};

	VT_NODISCARD VT_INLINE bool Asset_New::IsFlagSet(AssetFlag flag) const
	{
		AssetFlag value = static_cast<AssetFlag>(m_assetFlags.load(std::memory_order::relaxed) & static_cast<uint8_t>(flag));
		return value != AssetFlag::None;
	}

	VT_INLINE void Asset_New::SetName(const std::string& name)
	{
		if (name == m_name)
		{
			return;
		}

		m_name = name;
		OnAssetNameChanged();
	}

	VT_INLINE void Asset_New::SetFlag(AssetFlag flag, bool state)
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

	VT_INLINE void Asset_New::AssignAssetHandle(AssetHandle assetHandle)
	{
		m_handle = assetHandle;
	}
}
