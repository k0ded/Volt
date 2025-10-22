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
			// Make sure the asset manager isn't accessing the asset.
			if (m_asset)
			{
				m_asset->m_assetManagerLock.wait(true, std::memory_order::acquire);
				m_asset->m_assetReferenceLock.fetch_add(1u, std::memory_order::relaxed);
			}
		}

		~AssetReference() noexcept
		{
			if (m_asset)
			{
				// If we hold the last reference, make sure we notify all potential waiting threads
				// that the asset now can be accessed.
				uint32_t value = m_asset->m_assetReferenceLock.fetch_sub(1u, std::memory_order::release);
				if (value == 1)
				{
					std::atomic_thread_fence(std::memory_order::acquire);
					m_asset->m_assetReferenceLock.notify_all();
				}
			}
		}

		AssetReference(AssetReference&& other) noexcept
		{
			// When moving, we "move" the reference to this object, thus we don't need to increment the counter.
			m_asset = other.m_asset;
			other.m_asset = nullptr;
		}

		AssetReference(const AssetReference& other) noexcept
		{
			// When copying, we need to add a reference to the counter.
			m_asset = other.m_asset;
			if (m_asset)
			{
				m_asset->m_assetReferenceLock.fetch_add(1u, std::memory_order::relaxed);
			}
		}

		AssetReference& operator=(AssetReference&& other) noexcept
		{
			// When moving, we "move" the reference to this object, thus we don't need to increment the counter.
			m_asset = other.m_asset;
			other.m_asset = nullptr;
		 
			return *this;
		}

		AssetReference& operator=(const AssetReference& other) noexcept
		{
			// When copying, we need to add a reference to the counter.
			m_asset = other.m_asset;
			if (m_asset)
			{
				m_asset->m_assetReferenceLock.fetch_add(1u, std::memory_order::relaxed);
			}

			return *this;
		}

		T* operator->() noexcept
		{
			return m_asset.GetRaw();
		}

		T& operator*() noexcept
		{
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

	private:
		RefPtr<T> m_asset;
	};
}
