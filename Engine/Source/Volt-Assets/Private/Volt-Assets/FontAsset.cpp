#include "vtassetspch.h"

#include "Volt-Assets/FontAsset.h"

#include <Volt-Renderer/Texture/TextureSerializerCommon.h>

namespace Volt
{
	struct TextureHeader
	{
		RHI::PixelFormat format; // Should be one of the BC formats
		Vector<TextureSerializerCommon::TextureMip> mips;

		VT_INLINE friend Archive& operator<<(Archive& archive, TextureHeader& value)
		{
			uint32_t formatUint = static_cast<uint32_t>(value.format);

			archive << formatUint;
			archive << value.mips;

			if (archive.IsLoading())
			{
				value.format = static_cast<RHI::PixelFormat>(formatUint);
			}

			return archive;
		}
	};

	void FontAsset::Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata)
	{
		TextureHeader textureHeader;
		DataBuffer dataBuffer;

		if (!archive.IsLoading())
		{
			textureHeader.format = m_atlas->GetDesc().format;
			dataBuffer = TextureSerializerCommon::GetImageDataBuffer(m_atlas, textureHeader.mips);
		}

		archive << textureHeader;
		archive << dataBuffer;

		if (archive.IsLoading())
		{
			RHI::ImageDesc specification{};
			specification.format = textureHeader.format;
			specification.usage = RHI::ImageUsage::Texture;
			specification.width = textureHeader.mips.front().width;
			specification.height = textureHeader.mips.front().height;
			specification.mips = static_cast<uint32_t>(textureHeader.mips.size());
			specification.debugName = GetAssetName();
			specification.initializeImage = false;

			m_atlas = RHI::Image::Create(specification);

			TextureSerializerCommon::UploadImageData(m_atlas, textureHeader.format, textureHeader.mips, dataBuffer);
		}

		archive << m_metrics;
		archive << m_fontGeometry;
	}
}
