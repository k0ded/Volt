#include "rcpch.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphTexture.h"

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

	bool RGTexture::HasProducer(RGResourceUAV* uav) const
	{
		bool layersProduced = true;
		bool mipsProduced = true;

		const RGTextureUAVDesc& uavDesc = reinterpret_cast<RGTextureUAV*>(uav)->GetDesc();
		if (uavDesc.baseArrayLayer == 0 && uavDesc.layerCount == RHI::ImageViewDesc::LayerCountMax)
		{
			layersProduced = m_layersProduced.test(0);
		}
		else
		{
			for (uint32_t i = uavDesc.baseArrayLayer; i < uavDesc.baseArrayLayer + uavDesc.layerCount; ++i)
			{
				layersProduced &= m_layersProduced.test(i + 1);
			}
		}

		if (uavDesc.baseMipLevel == 0 && uavDesc.mipCount == RHI::ImageViewDesc::LayerCountMax)
		{
			mipsProduced = m_mipsProduced.test(0);
		}
		else
		{
			for (uint32_t i = uavDesc.baseMipLevel; i < uavDesc.baseMipLevel + uavDesc.mipCount; ++i)
			{
				mipsProduced &= m_layersProduced.test(i + 1);
			}
		}

		return layersProduced && mipsProduced;
	}

	bool RGTexture::HasProducer() const
	{
		return m_mipsProduced.any() && m_layersProduced.any();
	}

	void RGTexture::AddProducer(Handle<RenderGraphPass> pass, RGResourceUAV* uav)
	{
		const RGTextureUAVDesc& uavDesc = reinterpret_cast<RGTextureUAV*>(uav)->GetDesc();
		if (uavDesc.baseArrayLayer == 0 && uavDesc.layerCount == RHI::ImageViewDesc::LayerCountMax)
		{
			// Bit zero represents that the entire resource is produced
			m_layersProduced.set(0);
		}
		else
		{
			for (uint32_t i = uavDesc.baseArrayLayer; i < uavDesc.baseArrayLayer + uavDesc.layerCount; ++i)
			{
				m_layersProduced.set(i + 1);
			}
		}

		if (uavDesc.baseMipLevel == 0 && uavDesc.mipCount == RHI::ImageViewDesc::MipCountMax)
		{
			// Bit zero represents that the entire resource is produced
			m_mipsProduced.set(0);
		}
		else
		{
			for (uint32_t i = uavDesc.baseMipLevel; i < uavDesc.baseMipLevel + uavDesc.mipCount; ++i)
			{
				m_mipsProduced.set(i + 1);
			}
		}

		producers.emplace_back(pass);
	}

	void RGTexture::AddProducer(Handle<RenderGraphPass> pass)
	{
		// Setting bit zero means that the entire resource has been produced.
		m_layersProduced.set(0, true);
		m_mipsProduced.set(0, true);

		producers.emplace_back(pass);
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
