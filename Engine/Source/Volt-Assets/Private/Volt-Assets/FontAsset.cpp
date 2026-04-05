#include "vtassetspch.h"

#include "Volt-Assets/FontAsset.h"

#include <Volt-Renderer/Texture/TextureSerializerCommon.h>

#include <AssetSystem/AssetFactory.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::Font, FontAsset);

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

	FontAsset::FontAsset()
	{

	}

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

	glm::vec2 FontAsset::CalcTextSize(StringView string, float size)
	{
		if (string.empty())
		{
			return glm::vec2(0.f);
		}

		U32String utf32string(U32String::CtorConvert(), string.data(), string.size());

		const double fsScale = 1.0 / (m_metrics.ascenderY - m_metrics.descenderY);

		double maxLineWidth = 0.0;
		double sX = 0.0;
		int32_t lineCount = 1;

		for (int32_t i = 0; i < static_cast<int32_t>(utf32string.size()); i++)
		{
			char32_t character = utf32string[i];
			if (character == '\n')
			{
				maxLineWidth = glm::max(maxLineWidth, sX);
				sX = 0.0;
				lineCount++;
				continue;
			}

			const GlyphGeometry* glyph = m_fontGeometry.GetGlyph(character);
			if (!glyph)
			{
				glyph = m_fontGeometry.GetGlyph('?');
			}
			VT_ENSURE(glyph);

			double advance = glyph->GetAdvance();
			if (i + 1 < static_cast<int32_t>(utf32string.size()))
			{
				m_fontGeometry.GetAdvance(advance, character, utf32string[i + 1]);
			}
			sX += fsScale * advance;
		}
		maxLineWidth = glm::max(maxLineWidth, sX);

		const double lineHeight = fsScale * m_metrics.lineHeight;
		const double totalHeight = 1.0 + (lineCount - 1) * lineHeight;

		return glm::vec2(static_cast<float>(maxLineWidth) * size, static_cast<float>(totalHeight) * size);
	}
}
