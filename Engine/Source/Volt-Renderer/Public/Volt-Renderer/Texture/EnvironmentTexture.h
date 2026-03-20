#pragma once

#include "Volt-Renderer/Config.h"

#include <AssetSystem/AssetTypes.h>

#include <AssetSystem/Asset.h>

#include <RHIModule/Images/Image.h>

namespace Volt
{
	class VTR_API EnvironmentTexture : public Asset
	{
	public:
		EnvironmentTexture() = default;
		EnvironmentTexture(IntRef<RHI::Image> diffuseImage, IntRef<RHI::Image> specularImage);

		VT_NODISCARD VT_INLINE IntRef<RHI::Image> GetDiffuseImage() const { return m_diffuseImage; }
		VT_NODISCARD VT_INLINE IntRef<RHI::Image> GetSpecularImage() const { return m_specularImage; }

		static AssetType GetStaticType() { return AssetTypes::EnvironmentTexture; }
		AssetType GetType() const override { return GetStaticType(); }
		uint32_t GetVersion() const override { return 1; }
		void Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata) override;

	private:
		IntRef<RHI::Image> m_diffuseImage;
		IntRef<RHI::Image> m_specularImage;
	};
}
