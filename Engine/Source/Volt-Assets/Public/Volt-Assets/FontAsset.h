#pragma once

#include "Volt-Assets/Config.h"
#include "Volt-Assets/FontCommon.h"

#include <RHIModule/Images/Image.h>

#include <AssetSystem/AssetTypes.h>
#include <AssetSystem/Asset.h>

namespace Volt
{
	class FontAsset : public Asset
	{
	public:
		FontAsset() = default;
		~FontAsset() override = default;

		static AssetType GetStaticType() { return AssetTypes::Font; }
		AssetType GetType() const override { return GetStaticType(); };
		uint32_t GetVersion() const override { return 1; }
		void Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata) override;

	private:
		friend class FontSourceImporter;

		RefPtr<RHI::Image> m_atlas;
		FontMetrics m_metrics;
		FontGeometry m_fontGeometry;
	};
}
