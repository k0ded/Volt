#include "aspch.h"

#include "AssetSystem/Asset.h"
#include "AssetSystem/AssetManager.h"

#include <CoreUtilities/Archive/ArchiveVersionRegistry.h>

namespace Volt
{
	ArchiveVersionRegistrar g_registerAssetMetadataArchiveVersion(AssetMetadataArchiveVersion::guid, AssetMetadataArchiveVersion::LatestVersion, "AssetMetadataArchiveVersion");
	ArchiveVersionRegistrar g_registerCustomAssetMetadataArchiveVersion(CustomAssetMetadataArchiveVersion::guid, CustomAssetMetadataArchiveVersion::LatestVersion, "CustomAssetMetadataArchiveVersion");

	void AssetRefCounter::Unload() const
	{
		VT_ENSURE(m_referencedAssetManager != nullptr);
		m_referencedAssetManager->QueueAssetForDestruction(const_cast<AssetRefCounter*>(this));
	}
}
