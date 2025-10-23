#pragma once

#include "AssetSystem/AssetManagerCommon.h"

namespace Volt
{
	template<typename T>
	class AssetReference
	{
	public:
		AssetReference() = default;

		AssetReference(RefPtr<T> asset) noexcept
			: m_asset(asset)
		{
		}

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

		T* operator->() noexcept
		{
			VT_ENSURE(m_isLocked);
			return m_asset.GetRaw();
		}

		T& operator*() noexcept
		{
			VT_ENSURE(m_isLocked);
			return *m_asset;
		}
		
		bool operator==(std::nullptr_t) const noexcept
		{
			return m_asset == nullptr;
		}

		bool operator==(const AssetReference& other) noexcept
		{
			return m_asset == other.m_asset;
		}

		void Lock()
		{
			VT_ENSURE(m_asset != nullptr);
			VT_ENSURE(m_isLocked == false);
			m_asset->m_assetMutex.lock_shared();
			m_isLocked = true;
		}

		void Unlock()
		{
			VT_ENSURE(m_asset != nullptr);
			VT_ENSURE(m_isLocked == true);
			m_asset->m_assetMutex.unlock_shared();
			m_isLocked = false;
		}

	private:
		RefPtr<T> m_asset;
		bool m_isLocked = false;
	};
}
