#include "vrpch.h"

#include "Volt-Renderer/Texture/EnvironmentTexture.h"
#include "Volt-Renderer/Texture/TextureSerializerCommon.h"

#include <AssetSystem/AssetFactory.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::EnvironmentTexture, EnvironmentTexture);

	struct EnvironmentTextureHeader
	{
		RHI::PixelFormat format;
		Vector<TextureSerializerCommon::TextureMip> mips;
		uint32_t numLayers;

		VT_INLINE friend Archive& operator<<(Archive& archive, EnvironmentTextureHeader& value)
		{
			uint32_t formatUint = static_cast<uint32_t>(value.format);

			archive << formatUint;
			archive << value.mips;
			archive << value.numLayers;

			if (archive.IsLoading())
			{
				value.format = static_cast<RHI::PixelFormat>(formatUint);
			}

			return archive;
		}
	};

	EnvironmentTexture::EnvironmentTexture(IntRef<RHI::Image> diffuseImage, IntRef<RHI::Image> specularImage)
		: m_diffuseImage(diffuseImage), m_specularImage(specularImage)
	{

	}

	void EnvironmentTexture::Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata)
	{
		EnvironmentTextureHeader diffuseHeader{};
		EnvironmentTextureHeader specularHeader{};

		DataBuffer diffuseDataBuffer;
		DataBuffer specularDataBuffer;

		if (!archive.IsLoading())
		{
			const RHI::ImageDesc& diffuseImageDesc = m_diffuseImage->GetDesc();
			const RHI::ImageDesc& specularImageDesc = m_specularImage->GetDesc();

			diffuseHeader.format = diffuseImageDesc.format;
			diffuseHeader.numLayers = diffuseImageDesc.layers;
			specularHeader.format = specularImageDesc.format;
			specularHeader.numLayers = specularImageDesc.layers;

			diffuseDataBuffer = TextureSerializerCommon::GetImageDataBuffer(m_diffuseImage, diffuseHeader.mips);
			specularDataBuffer = TextureSerializerCommon::GetImageDataBuffer(m_specularImage, specularHeader.mips);
		}

		archive << diffuseHeader;
		archive << specularHeader;
		archive << diffuseDataBuffer;
		archive << specularDataBuffer;

		if (archive.IsLoading())
		{
			RHI::ImageDesc specification{};
			specification.format = diffuseHeader.format;
			specification.width = diffuseHeader.mips.front().width;
			specification.height = diffuseHeader.mips.front().height;
			specification.layers = diffuseHeader.numLayers;
			specification.mips = static_cast<uint32_t>(diffuseHeader.mips.size());
			specification.usage = RHI::ImageUsage::Texture;
			specification.isCubeMap = true;
			specification.debugName = GetAssetName();

			m_diffuseImage = RHI::Image::Create(specification);

			specification.format = specularHeader.format;
			specification.width = specularHeader.mips.front().width;
			specification.height = specularHeader.mips.front().height;
			specification.layers = specularHeader.numLayers;
			specification.mips = static_cast<uint32_t>(specularHeader.mips.size());

			m_specularImage = RHI::Image::Create(specification);

			TextureSerializerCommon::UploadImageData(m_diffuseImage, diffuseHeader.format, diffuseHeader.mips, diffuseDataBuffer);
			TextureSerializerCommon::UploadImageData(m_specularImage, specularHeader.format, specularHeader.mips, specularDataBuffer);
		}
	}
}
