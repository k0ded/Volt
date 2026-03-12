#include "aspch.h"
#include "AssetFactory.h"

Volt::AssetFactory g_assetFactory;

namespace Volt
{
	Volt::AssetFactory& AssetFactory::Get()
	{
		return g_assetFactory;
	}
}
