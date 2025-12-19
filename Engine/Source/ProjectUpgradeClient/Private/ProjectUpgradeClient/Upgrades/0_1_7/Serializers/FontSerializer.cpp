#include "ProjectUpgradeClient/Upgrades/0_1_7/Serializers/FontSerializer.h"

#include <Volt-Assets/Font.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

namespace Volt
{
	void FontSerializer::Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		VT_ASSERT_MSG(false, "[FontSerializer]: Asset it not serializable");
	}

	bool FontSerializer::Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const
	{
		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);

		if (!std::filesystem::exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata->filepath);
			destinationAsset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		AssetReference<Font> font = destinationAsset.ConvertTo<Font>();
		ScopedAssetReferenceLock fontLock{ font };

		font->Initialize(filePath);

		return true;
	}
}
