#include "vtassetspch.h"

#include "Volt-Assets/Serializers/FontSerializer.h"
#include "Volt-Assets/Font.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

namespace Volt
{
	void FontSerializer::Serialize(ReadOnlyAssetMetadata metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		VT_ASSERT_MSG(false, "[FontSerializer]: Asset it not serializable");
	}

	bool FontSerializer::Deserialize(ReadOnlyAssetMetadata metadata, AssetReference<Asset> destinationAsset) const
	{
		const auto filePath = g_assetManager->GetFilesystemPath(metadata->filepath);

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
