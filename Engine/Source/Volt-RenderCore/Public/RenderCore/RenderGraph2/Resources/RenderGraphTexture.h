#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph2/Resources/RenderGraphResource.h"

#include <RHIModule/Core/RHICommon.h>

namespace Volt
{
	struct RGTextureDesc : public RHI::ImageSpecification
	{};

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
