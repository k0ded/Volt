#pragma once

#include "AssetSystem/AssetManagerCommon.h"

#include <CoreUtilities/Pointers/IntRef.h>

// Note: Kept outside of Volt namespace for ease of use.

template<typename T>
class AssetReference
{
public:
	AssetReference() = default;

	AssetReference(IntRef<T> asset) noexcept
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
		return m_asset.GetRaw();
	}

	VT_INLINE T& operator*() noexcept
	{
		return *m_asset;
	}

	VT_INLINE const T* operator->() const noexcept
	{
		return m_asset.GetRaw();
	}

	VT_INLINE const T& operator*() const noexcept
	{
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

	VT_INLINE void Reset()
	{
		m_asset = nullptr;
	}

	VT_INLINE bool IsValid() const
	{
		return m_asset != nullptr;
	}

	VT_INLINE IntRef<T> GetRaw() const
	{
		return m_asset;
	}

	template<std::derived_from<T> U>
	VT_INLINE AssetReference<U> ConvertTo() const
	{
		AssetReference<U> result{ m_asset.template As<U>() };
		return result;
	}

private:

	template<typename U>
	friend class AssetReference;

	IntRef<T> m_asset;
};
