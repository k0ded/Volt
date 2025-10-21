#include "aspch.h"

#include "AssetSystem/Asset_New.h"
#include "AssetSystem/AssetManager_New.h"

namespace Volt
{
	void AssetRefCounter::Unload() const
	{
		VT_ENSURE(m_referencedAssetManager != nullptr);
		m_referencedAssetManager->UnloadAndFreeAsset(const_cast<AssetRefCounter*>(this));
	}
}
