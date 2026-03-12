#include "aspch.h"

#include "AssetSystem/CustomAssetMetadataRegistry.h"
#include "AssetSystem/AssetMetadata.h"

namespace Volt
{
	void CustomAssetMetadataRegistry::UnregisterCustomMetadata(AssetType assetType)
	{
		// #Note_Ivar: The registry may already have been destroyed due to
		// DLL ordering.
		if (m_registry.empty())
		{
			return;
		}

		if (VT_CHECK(m_registry.contains(assetType)))
		{
			m_registry.erase(assetType);
		}
	}

	bool CustomAssetMetadataRegistry::AssetTypeHasCustomMetadata(AssetType assetType) const
	{
		return m_registry.contains(assetType);
	}

	void CustomAssetMetadataRegistry::SerializeAny(AssetType assetType, Any& value, Archive& archive) const
	{
		VT_ENSURE(m_registry.contains(assetType));
		m_registry.at(assetType).SerializeAny(value, archive);
	}

	void CustomAssetMetadataRegistry::SetupInitalCustomMetadata(AssetType assetType, CustomAssetMetadata& customMetadata) const
	{
		VT_ENSURE(m_registry.contains(assetType));
		m_registry.at(assetType).InitializeAny(customMetadata.GetStorage());
	}

	CustomAssetMetadataRegistry& CustomAssetMetadataRegistry::Get()
	{
		static CustomAssetMetadataRegistry registry;
		return registry;
	}
}
