#include "rcpch.h"

#include "RenderCore/RenderGraph2/Resources/RenderGraphTexture.h"

namespace Volt
{

	RGTexture::RGTexture(const RGTextureDesc& desc)
		: m_desc(desc)
	{

	}

	RGResourceType RGTexture::GetResourceType() const
	{
		return RGResourceType::Texture;
	}

	RGTextureSRV::RGTextureSRV(const RGTextureSRVDesc& desc)
		: m_desc(desc)
	{

	}

	RGTextureUAV::RGTextureUAV(const RGTextureUAVDesc& desc)
		: m_desc(desc)
	{

	}
}
