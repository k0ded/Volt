#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetManagerCommon.h"
#include "AssetSystem/AssetAllocatorCommon.h"

#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
	class AssetAllocator
	{
	public:
		AssetAllocator();

		template<VoltAssetType T, typename... Args> RefPtr<T> AllocateAsset(Args&&... args);
		RefPtr<Asset> AllocateAssetWithType(AssetType type);
		void FreeAsset(AssetType assetType, Asset* asset);
		void ReallocateAsset(AssetType assetType, Asset* asset);

	private:
		void InitializeAllocators();

		Map<VoltGUID, Ref<AssetTypeAllocator>> m_assetAllocator;
	};

	template<VoltAssetType T, typename... Args>
	RefPtr<T> AssetAllocator::AllocateAsset(Args&&... args)
	{
		static const AssetType assetType = T::GetStaticType();
		VT_ENSURE(m_assetAllocator.contains(assetType->GetGUID()));

		// This is safe because the type has been registered to this guid.
		AssetTypeAllocatorImpl<T>& allocator = *std::reinterpret_pointer_cast<AssetTypeAllocatorImpl<T>>(m_assetAllocator.at(assetType->GetGUID()));

		T* assetPtr = allocator.Allocate(std::forward<Args>(args)...);
		assetPtr->AssignAssetHandle(AssetHandle{});

		return RefPtr<T>::AttachNoRef(assetPtr);
	}
}
