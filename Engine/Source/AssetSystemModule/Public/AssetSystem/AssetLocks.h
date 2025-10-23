#pragma once

#include "AssetSystem/AssetReference.h"

namespace Volt
{
	template<typename T>
	class ScopedAssetReferenceLock
	{
	public:
		ScopedAssetReferenceLock(AssetReference<T>& assetReference) noexcept
			: m_assetReference(assetReference)
		{
			assetReference.Lock();
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
		AssetReference<T>& m_assetReference;
	};
}
