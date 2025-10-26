#pragma once

#include "AssetSystem/AssetReference.h"

namespace Volt
{
	template<typename T>
	class ScopedAssetReferenceLock
	{
	public:
		ScopedAssetReferenceLock(const AssetReference<T>& assetReference) noexcept
			: m_assetReference(assetReference)
		{
			m_assetReference.Lock();
		}

		~ScopedAssetReferenceLock() noexcept
		{
			m_assetReference.Unlock();
		}

		ScopedAssetReferenceLock(ScopedAssetReferenceLock&&) = delete;
		ScopedAssetReferenceLock(const ScopedAssetReferenceLock&) = delete;
		ScopedAssetReferenceLock& operator=(ScopedAssetReferenceLock&&) = delete;
		ScopedAssetReferenceLock& operator=(const ScopedAssetReferenceLock&) = delete;

	private:
		const AssetReference<T>& m_assetReference;
	};
}
