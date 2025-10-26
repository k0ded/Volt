#include "vrpch.h"

#include "Volt-Renderer/Texture/Texture2D.h"

#include <AssetSystem/AssetFactory.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::Texture, Texture2D);

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

	Ref<Texture2D> Texture2D::Create(RHI::PixelFormat format, uint32_t width, uint32_t height, const void* data)
	{
		return CreateRef<Texture2D>(format, width, height, data);
	}

	Ref<Texture2D> Texture2D::Create(RefPtr<RHI::Image> image)
	{
		return CreateRef<Texture2D>(image);
	}
}
