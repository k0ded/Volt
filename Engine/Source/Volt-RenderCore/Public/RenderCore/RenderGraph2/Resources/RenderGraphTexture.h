#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph2/Resources/RenderGraphResource.h"

#include <RHIModule/Core/RHICommon.h>

namespace Volt
{
	struct RGTextureDesc : public RHI::ImageSpecification
	{
		template<RHI::PixelFormat PixelFormat>
		static RGTextureDesc Create2D(const uint32_t width, const uint32_t height, RHI::ImageUsage usage, const std::string& name = "Texture")
		{
			VT_ASSERT_MSG(width > 0 && height > 0, "Width and height must not be zero!");
			
			RGTextureDesc resultDesc{};
			resultDesc.width = width;
			resultDesc.height = height;
			resultDesc.usage = usage;
			resultDesc.debugName = name;
			resultDesc.imageType = RHI::ResourceType::Image2D;

			return resultDesc;
		}
	};

	class VTRC_API RGTexture : public RGResource
	{
	public:
		RGTexture(const RGTextureDesc& desc);
		~RGTexture() override = default;
		RGResourceType GetResourceType() const override;

		VT_NODISCARD VT_INLINE const RGTextureDesc& GetDesc() const { return m_desc; }
	
	private:
		RGTextureDesc m_desc;
	};

	using RGTextureRef = RGTexture*;

	struct RGTextureSRVDesc
	{ 
		RGTextureRef textureResource;
	};

	class VTRC_API RGTextureSRV : public RGResourceSRV
	{
	public:
		RGTextureSRV(const RGTextureSRVDesc& desc);
		~RGTextureSRV() override = default;

		RGResourceRef GetResource() const override { return m_desc.textureResource; }

	private:
		RGTextureSRVDesc m_desc;
	};

	struct RGTextureUAVDesc
	{
		RGTextureRef textureResource;
	};

	class VTRC_API RGTextureUAV : public RGResourceUAV
	{
	public:
		RGTextureUAV(const RGTextureUAVDesc& desc);
		~RGTextureUAV() override = default;

		RGResourceRef GetResource() const override { return m_desc.textureResource; }

	private:
		RGTextureUAVDesc m_desc;
	};
}
