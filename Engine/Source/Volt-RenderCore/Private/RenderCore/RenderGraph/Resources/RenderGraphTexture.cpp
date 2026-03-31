#include "rcpch.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphTexture.h"
#include "RenderCore/TransientResourceSystem/TransientResource.h"

#include <RHIModule/RHIHelpers.h>
#include <RHIModule/Graphics/GraphicsContext.h>

namespace Volt
{
	RGTexture::RGTexture(const RGTextureDesc& desc, uint32_t resourceId, RenderGraphDataAllocator* dataAllocator, bool isExternal)
		: RGResource(resourceId),
		m_desc(desc)
	{
		m_isTransient = desc.memoryUsage == RHI::MemoryUsage::GPU;
	
		if (!isExternal)
		{
			m_memoryRequirement = RHI::GraphicsContext::GetDevice()->GetImageMemoryRequirement(m_desc);
		}

		lastAccess.set_allocator({ dataAllocator });
		firstAccess.set_allocator({ dataAllocator });
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
