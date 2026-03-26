#pragma once

#include "Volt-Assets/Config.h"
#include "Volt-Assets/FontCommon.h"

#include <RHIModule/Images/Image.h>

#include <AssetSystem/AssetTypes.h>
#include <AssetSystem/Asset.h>

namespace Volt
{
	class VTASSETS_API FontAsset : public Asset
	{
	public:
		FontAsset() = default;
		~FontAsset() override = default;

		VT_NODISCARD VT_INLINE const FontMetrics& GetMetrics() const { return m_metrics; }
		VT_NODISCARD VT_INLINE const FontGeometry& GetGeometry() const { return m_fontGeometry; }
		VT_NODISCARD VT_INLINE IntRef<RHI::Image> GetAtlas() const { return m_atlas; }

		static AssetType GetStaticType() { return AssetTypes::Font; }
		AssetType GetType() const override { return GetStaticType(); };
		uint32_t GetVersion() const override { return 1; }
		void Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata) override;

		glm::vec2 CalcTextSize(std::string_view string, float size);

	private:
		friend class FontSourceImporter;

		IntRef<RHI::Image> m_atlas;
		FontMetrics m_metrics;
		FontGeometry m_fontGeometry;
	};
}
