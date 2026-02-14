#include "rcpch.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphTexture.h"

#include <RHIModule/RHIHelpers.h>

namespace Volt
{

	RGTexture::RGTexture(const RGTextureDesc& desc)
		: m_desc(desc)
	{
		InitializeSubResources();
	}

	RGResourceType RGTexture::GetResourceType() const
	{
		return RGResourceType::Texture;
	}

	void RGTexture::InitializeSubResources()
	{
		const uint32_t numSubResources = m_desc.mips * m_desc.layers;
		lastAccess.resize(numSubResources);
		firstAccess.resize(numSubResources, nullptr);
	}

	RGTextureSRV::RGTextureSRV(const RGTextureSRVDesc& desc)
		: m_desc(desc)
	{
		VT_ENSURE(m_desc.baseArrayLayer + m_desc.layerCount <= RHI::ImageViewDesc::LayerCountMax);
	}

	RGTextureUAV::RGTextureUAV(const RGTextureUAVDesc& desc)
		: m_desc(desc)
	{
		VT_ENSURE(m_desc.baseMipLevel + m_desc.mipCount <= RHI::ImageViewDesc::MipCountMax);
	}
}
