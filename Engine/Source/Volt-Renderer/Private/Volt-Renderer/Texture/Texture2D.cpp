#include "vrpch.h"

#include "Volt-Renderer/Texture/Texture2D.h"
#include "Volt-Renderer/Texture/TextureSerializerCommon.h"

#include <AssetSystem/AssetFactory.h>

#include <RenderCore/CommandBufferPool.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/Utility/ResourceUtility.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::Texture, Texture2D);

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

	Texture2D::Texture2D(RHI::PixelFormat format, uint32_t width, uint32_t height, const void* data)
	{
		RHI::ImageDesc imageSpec{};
		imageSpec.format = format;
		imageSpec.usage = RHI::ImageUsage::Texture;
		imageSpec.width = static_cast<uint32_t>(width);
		imageSpec.height = static_cast<uint32_t>(height);

		m_image = RHI::Image::Create(imageSpec, data);
	}

	Texture2D::Texture2D(RefPtr<RHI::Image> image)
		: m_image(image)
	{
	}

	Texture2D::~Texture2D()
	{
		m_image = nullptr;
	}

	const uint32_t Texture2D::GetWidth() const
	{
		return m_image->GetWidth();
	}

	const uint32_t Texture2D::GetHeight() const
	{
		return m_image->GetHeight();
	}

	void Texture2D::SetImage(RefPtr<RHI::Image> image)
	{
		m_image = image;
	}

	void Texture2D::Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata)
	{
		TextureHeader textureHeader;
		DataBuffer dataBuffer;

		if (!archive.IsLoading())
		{
			textureHeader.format = m_image->GetDesc().format;
			dataBuffer = TextureSerializerCommon::GetImageDataBuffer(m_image, textureHeader.mips);
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
			specification.generateMips = false;
			specification.debugName = GetAssetName();
			specification.initializeImage = false;

			m_image = RHI::Image::Create(specification);

			TextureSerializerCommon::UploadImageData(m_image, textureHeader.format, textureHeader.mips, dataBuffer);
		}
	}

	Ref<Texture2D> Texture2D::Create(RHI::PixelFormat format, uint32_t width, uint32_t height, const void* data)
	{
		return CreateRef<Texture2D>(format, width, height, data);
	}

	Ref<Texture2D> Texture2D::Create(RefPtr<RHI::Image> image)
	{
		return CreateRef<Texture2D>(image);
	}
}
