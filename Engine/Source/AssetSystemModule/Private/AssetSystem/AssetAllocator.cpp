#include "aspch.h"

#include "AssetSystem/AssetAllocator.h"
#include "AssetSystem/AssetFactory.h"

namespace Volt
{

	AssetAllocator::AssetAllocator()
	{
		InitializeAllocators();
	}

	void AssetAllocator::InitializeAllocators()
	{
		AssetFactory& assetFactory = AssetFactory::Get();
	
		// Initialize all the asset types allocators.
		for (const auto& [guid, factoryData] : assetFactory.GetFactoryMap())
		{
			m_assetAllocator[guid] = factoryData.createAllocatorFunction();
		}
	}

	RefPtr<Asset> AssetAllocator::AllocateAssetWithType(AssetType type)
	{
		VT_ENSURE(m_assetAllocator.contains(type->GetGUID()));

		Ref<AssetTypeAllocator> allocator = m_assetAllocator.at(type->GetGUID());

		Asset* assetPtr = allocator->AllocateDefault();
		assetPtr->AssignAssetHandle(AssetHandle{});

		return RefPtr<Asset>::AttachNoRef(assetPtr);
	}

	void AssetAllocator::FreeAsset(AssetType assetType, Asset* asset)
	{
		const VoltGUID assetTypeGUID = assetType->GetGUID();
		VT_ENSURE(m_assetAllocator.contains(assetTypeGUID));

		m_assetAllocator.at(assetTypeGUID)->Free(asset);
	}

	void AssetAllocator::ReallocateAsset(AssetType assetType, Asset* asset)
	{
		const VoltGUID assetTypeGUID = assetType->GetGUID();
		VT_ENSURE(m_assetAllocator.contains(assetTypeGUID));

		m_assetAllocator.at(assetTypeGUID)->Reallocate(asset);
	}
}
