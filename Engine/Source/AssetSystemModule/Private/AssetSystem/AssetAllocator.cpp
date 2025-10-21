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

	void AssetAllocator::FreeAsset(AssetType assetType, Asset_New* asset)
	{
		const VoltGUID assetTypeGUID = assetType->GetGUID();
		VT_ENSURE(m_assetAllocator.contains(assetTypeGUID));

		m_assetAllocator.at(assetTypeGUID)->Free(asset);
	}
}
