#pragma once

#include "AssetSystem/AssetManagerCommon.h"

#include <CoreUtilities/Pointers/RefPtr.h>

// Note: Kept outside of Volt namespace for ease of use.

template<typename T>
class AssetReference
{
public:
	AssetReference() = default;

	AssetReference(RefPtr<T> asset) noexcept
		: m_asset(asset)
	{}

	AssetReference(std::nullptr_t) noexcept
	{}

	template<std::derived_from<T> U>
	AssetReference(const AssetReference<U>& other) noexcept
		: m_asset(other.m_asset)
	{}

	~AssetReference() noexcept
	{
		VT_ENSURE_MSG(m_isLocked == false, "Must be unlocked before destroyed!");
	}

	AssetReference(AssetReference&& other) noexcept
	{
		m_asset = other.m_asset;
		other.m_asset = nullptr;
	}

	AssetReference(const AssetReference& other) noexcept
	{
		m_asset = other.m_asset;
	}

	AssetReference& operator=(AssetReference&& other) noexcept
	{
		m_asset = other.m_asset;
		other.m_asset = nullptr;

		return *this;
	}

	AssetReference& operator=(const AssetReference& other) noexcept
	{
		m_asset = other.m_asset;
		return *this;
	}

	AssetReference& operator=(std::nullptr_t) noexcept
	{
		Reset();
		return *this;
	}

	VT_INLINE T* operator->() noexcept
	{
		VT_ENSURE_MSG(m_isLocked, "AssetReference must be locked when accessed!");
		return m_asset.GetRaw();
	}

	VT_INLINE T& operator*() noexcept
	{
		VT_ENSURE_MSG(m_isLocked, "AssetReference must be locked when accessed!");
		return *m_asset;
	}

	VT_INLINE const T* operator->() const noexcept
	{
		VT_ENSURE_MSG(m_isLocked, "AssetReference must be locked when accessed!");
		return m_asset.GetRaw();
	}

	VT_INLINE const T& operator*() const noexcept
	{
		VT_ENSURE_MSG(m_isLocked, "AssetReference must be locked when accessed!");
		return *m_asset;
	}

	VT_INLINE bool operator==(std::nullptr_t) const noexcept
	{
		return m_asset == nullptr;
	}

	VT_INLINE bool operator==(const AssetReference& other) noexcept
	{
		return m_asset == other.m_asset;
	}

	VT_INLINE explicit operator bool() const
	{
		return m_asset != nullptr;
	}

	VT_INLINE void Lock() const
	{
		VT_ENSURE(m_asset != nullptr);
		VT_ENSURE(m_isLocked == false);
		m_asset->m_assetMutex.lock_shared();
		m_isLocked = true;
	}

	VT_INLINE void Unlock() const
	{
		VT_ENSURE(m_asset != nullptr);
		VT_ENSURE(m_isLocked == true);
		m_asset->m_assetMutex.unlock_shared();
		m_isLocked = false;
	}

	VT_INLINE void Reset()
	{
		VT_ENSURE(m_isLocked == false);
		m_asset = nullptr;
	}

	VT_INLINE bool IsValid() const
	{
		return m_asset != nullptr;
	}

	VT_INLINE RefPtr<T> GetRaw() const
	{
		return m_asset;
	}

	template<std::derived_from<T> U>
	VT_INLINE AssetReference<U> ConvertTo() const
	{
		AssetReference<U> result{ m_asset.As<U>() };
		return result;
	}

private:

	template<typename U>
	friend class AssetReference;

	RefPtr<T> m_asset;
	mutable bool m_isLocked = false;
};
