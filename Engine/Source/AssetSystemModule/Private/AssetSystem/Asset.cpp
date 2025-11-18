#include "aspch.h"

#include "AssetSystem/Asset.h"
#include "AssetSystem/AssetManager.h"

namespace Volt
{
	void AssetRefCounter::Unload() const
	{
		VT_ENSURE(m_referencedAssetManager != nullptr);
		m_referencedAssetManager->QueueAssetForDestruction(const_cast<AssetRefCounter*>(this));
	}
}
