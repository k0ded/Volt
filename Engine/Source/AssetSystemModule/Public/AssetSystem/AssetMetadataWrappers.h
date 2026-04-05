#pragma once

#include "AssetSystem/AssetMetadata.h"

namespace Volt
{
	enum class AssetMetadataInit
	{
		Null
	};

	// A wrapper of the asset metadata that ensures no other
	// thread can access it at the same time.
	// Takes a asset metadata as a reference, it is ok as all metadata
	// has stable pointers.
	class WriteableAssetMetadata
	{
	public:
		WriteableAssetMetadata(AssetMetadataInit) noexcept
			: m_metadata(nullptr)
		{}

		WriteableAssetMetadata(AssetMetadata* inMetadata) noexcept
			: m_metadata(inMetadata)
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.lock();
			}
		}

		~WriteableAssetMetadata() noexcept
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.unlock();
			}
		}

		WriteableAssetMetadata(WriteableAssetMetadata&&) = delete;
		WriteableAssetMetadata(const WriteableAssetMetadata&) = delete;
		WriteableAssetMetadata& operator=(WriteableAssetMetadata&&) = delete;
		WriteableAssetMetadata& operator=(const WriteableAssetMetadata&) = delete;

		VT_INLINE AssetMetadata* operator->() noexcept
		{
			return m_metadata;
		}

		VT_INLINE AssetMetadata& operator*() noexcept
		{
			return *m_metadata;
		}

		VT_INLINE bool IsValid() const
		{
			return m_metadata != nullptr;
		}

	private:
		AssetMetadata* m_metadata;
	};

	// A wrapper of the asset metadata that allows multiple threads to read the same
	// asset metadata at the same time. Will make sure asset metadata
	// can't be written at the same time as it's being read.
	class ReadOnlyAssetMetadata
	{
	public:
		ReadOnlyAssetMetadata(AssetMetadataInit) noexcept
			: m_metadata(nullptr)
		{}

		ReadOnlyAssetMetadata(AssetMetadata* inMetadata) noexcept
			: m_metadata(inMetadata)
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.lock_shared();
			}
		}

		~ReadOnlyAssetMetadata() noexcept
		{
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.unlock_shared();
			}
		}

		VT_INLINE ReadOnlyAssetMetadata(ReadOnlyAssetMetadata&& other) noexcept
		{
			// We take control of the lock from the moved asset.
			m_metadata = other.m_metadata;
			other.m_metadata = nullptr;
		}

		VT_INLINE ReadOnlyAssetMetadata(const ReadOnlyAssetMetadata& other) noexcept
		{
			// Lock the asset meta for this instance.
			m_metadata = other.m_metadata;
			if (m_metadata)
			{
				m_metadata->m_assetMetadataMutex.lock_shared();
			}
		}

		VT_INLINE ReadOnlyAssetMetadata& operator=(ReadOnlyAssetMetadata&& other) noexcept
		{
			if (this != &other)
			{
				if (m_metadata)
				{
					m_metadata->m_assetMetadataMutex.unlock_shared();
				}

				// We take control of the lock from the moved asset.
				m_metadata = other.m_metadata;
				other.m_metadata = nullptr;
			}

			return *this;
		}

		VT_INLINE ReadOnlyAssetMetadata& operator=(const ReadOnlyAssetMetadata& other) noexcept
		{
			if (this != &other)
			{
				if (m_metadata)
				{
					m_metadata->m_assetMetadataMutex.unlock_shared();
				}

				// Lock the asset meta for this instance.
				m_metadata = other.m_metadata;
				if (m_metadata)
				{
					m_metadata->m_assetMetadataMutex.lock_shared();
				}
			}

			return *this;
		}

		VT_INLINE const AssetMetadata* operator->() const noexcept
		{
			return m_metadata;
		}

		VT_INLINE const AssetMetadata& operator*() const noexcept
		{
			return *m_metadata;
		}

		VT_INLINE bool IsValid() const
		{
			return m_metadata != nullptr;
		}

	private:
		AssetMetadata* m_metadata;
	};
}
