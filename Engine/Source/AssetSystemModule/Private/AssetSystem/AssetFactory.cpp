#include "aspch.h"
#include "AssetFactory.h"

Volt::AssetFactory g_assetFactory;

namespace Volt
{
	Ref<Asset> AssetFactory::CreateAssetOfType(AssetType type) const
	{
		VT_ENSURE(m_assetFactoryFunctions.contains(type->GetGUID()));
		return m_assetFactoryFunctions.at(type->GetGUID()).createFunction();
	}

	Volt::AssetFactory& AssetFactory::Get()
	{
		return g_assetFactory;
	}
}
